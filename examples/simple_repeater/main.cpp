#include <Arduino.h>   // needed for PlatformIO
#include <Mesh.h>

#include "MyMesh.h"

#ifdef ETHERNET_ENABLED
  #define ETHERNET_CLI_BANNER "MeshCore Repeater CLI"
  #include <helpers/nrf52/EthernetCLI.h>
#endif

StdRNG fast_rng;
SimpleMeshTables tables;

MyMesh the_mesh(board, radio_driver, *new ArduinoMillis(), fast_rng, rtc_clock, tables);

void halt() {
  while (1) ;
}

static char command[160];
#ifdef ETHERNET_ENABLED
static char ethernet_command[160];
#endif

// For power saving
unsigned long POWERSAVING_FIRSTSLEEP_SECS = 120; // The first sleep (if enabled) from boot

#if defined(PIN_USER_BTN) && defined(_SEEED_SENSECAP_SOLAR_H_)
static unsigned long userBtnDownAt = 0;
#define USER_BTN_HOLD_OFF_MILLIS 1500
#endif

#if defined(THINKNODE_M6) && ENV_INCLUDE_GPS == 1 && !defined(DISPLAY_CLASS)
#include <helpers/ui/MomentaryButton.h>
// Elecrow M6 manual: a double-click updates the location, a triple-click
// turns the GPS on or off. ThinkNodeM6Board handles the long presses.
static MomentaryButton m6_btn(PIN_USER_BTN, 1000, true, true);
static unsigned long m6_fix_wait_until = 0;   // a double-click waits for a GPS fix until then
static unsigned long m6_fix_since = 0;        // when the current fix became valid
#define M6_FIX_WAIT_MILLIS   (5 * 60 * 1000UL)   // "gps once" gives up after 5 min too
#define M6_FIX_SETTLE_MILLIS 5000   // a first fix can be off; "gps once" keeps the GPS on 10 s after it

static void m6Command(const char* text) {
  char cmd[32];
  char reply[160];
  strcpy(cmd, text);
  reply[0] = 0;
  the_mesh.handleCommand(0, cmd, reply);
  Serial.printf("button: %s -> %s\n", text, reply);
}

static void m6ButtonLoop() {
  LocationProvider* gps = sensors.getLocationProvider();
  if (gps == NULL) {
    return;
  }
  int event = m6_btn.check();
  if (event == BUTTON_EVENT_DOUBLE_CLICK) {
    m6_fix_wait_until = millis() + M6_FIX_WAIT_MILLIS;
    if (!gps->isValid()) {
      m6Command("gps once");   // power the GPS until it gets a fix
    }
  } else if (event == BUTTON_EVENT_TRIPLE_CLICK) {
    m6Command(gps->isGpsOn() ? "gps off" : "gps on");
  }

  if (m6_fix_wait_until != 0) {
    bool fix = gps->isValid() && gps->hasValidFix();   // hasValidFix(): accepted by the GPS guard, if any
    if (!fix) {
      m6_fix_since = 0;
    } else if (m6_fix_since == 0) {
      m6_fix_since = millis();
    }
    if (fix && millis() - m6_fix_since >= M6_FIX_SETTLE_MILLIS) {
      // sensors only copy the fix every few seconds: take it now
      sensors.node_lat = gps->getLatitude() / 1000000.;
      sensors.node_lon = gps->getLongitude() / 1000000.;
      m6Command("gps setloc");
      m6Command("advert.zerohop");
      m6_fix_wait_until = 0;
      m6_fix_since = 0;
    } else if (the_mesh.millisHasNowPassed(m6_fix_wait_until)) {
      m6_fix_wait_until = 0;   // no fix in time: the saved position stays
      m6_fix_since = 0;
    }
  }
}
#endif

void setup() {
  Serial.begin(115200);
  delay(1000);

  board.begin();
#if defined(THINKNODE_M6) && ENV_INCLUDE_GPS == 1 && !defined(DISPLAY_CLASS)
  m6_btn.begin();
#endif

#ifdef HAS_EXTERNAL_WATCHDOG
  external_watchdog.begin();
#endif

#if defined(MESH_DEBUG) && defined(NRF52_PLATFORM)
  // give some extra time for serial to settle so
  // boot debug messages can be seen on terminal
  delay(5000);
#endif

#ifdef DISPLAY_CLASS
  if (display.begin()) {
    display.startFrame();
    display.setCursor(0, 0);
    display.print("Please wait...");
    display.endFrame();
  }
#endif

  if (!radio_init()) {
    MESH_DEBUG_PRINTLN("Radio init failed!");
    halt();
  }

  fast_rng.begin(radio_driver.getRngSeed());

  FILESYSTEM* fs;
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  InternalFS.begin();
  fs = &InternalFS;
  IdentityStore store(InternalFS, "");
#elif defined(ESP32)
  SPIFFS.begin(true);
  fs = &SPIFFS;
  IdentityStore store(SPIFFS, "/identity");
#elif defined(RP2040_PLATFORM)
  LittleFS.begin();
  fs = &LittleFS;
  IdentityStore store(LittleFS, "/identity");
  store.begin();
#else
  #error "need to define filesystem"
#endif
  if (!store.load("_main", the_mesh.self_id)) {
    MESH_DEBUG_PRINTLN("Generating new keypair");
    the_mesh.self_id = radio_new_identity();   // create new random identity
    int count = 0;
    while (count < 10 && (the_mesh.self_id.pub_key[0] == 0x00 || the_mesh.self_id.pub_key[0] == 0xFF)) {  // reserved id hashes
      the_mesh.self_id = radio_new_identity(); count++;
    }
    store.save("_main", the_mesh.self_id);
  }

  Serial.print("Repeater ID: ");
  mesh::Utils::printHex(Serial, the_mesh.self_id.pub_key, PUB_KEY_SIZE); Serial.println();

  command[0] = 0;
#ifdef ETHERNET_ENABLED
  ethernet_command[0] = 0;
#endif

  sensors.begin();

  the_mesh.begin(fs);

#ifdef DISPLAY_CLASS
  ui_task.begin(the_mesh.getNodePrefs(), FIRMWARE_BUILD_DATE, FIRMWARE_VERSION, "< Repeater >");
#endif

#ifdef ETHERNET_ENABLED
  ethernet_start_task();
#endif

  // send out initial zero hop Advertisement to the mesh
#if ENABLE_ADVERT_ON_BOOT == 1
  the_mesh.sendSelfAdvertisement(16000, false);
#endif

  board.onBootComplete();
}

void loop() {
  // Handle Serial CLI
  int len = strlen(command);
  // `command` must stay NUL-terminated within its bounds. If it ever isn't,
  // strlen() above can return >= sizeof(command) and the loop below would then
  // index past the buffer, so clamp defensively.
  if (len >= (int)sizeof(command)) {
    command[0] = 0;
    len = 0;
  }
  while (Serial.available() && len < sizeof(command)-1) {
    char c = Serial.read();
    if (c != '\n') {
      command[len++] = c;
      command[len] = 0;
      Serial.print(c);
    }
    if (c == '\r') break;
  }
  if (len == sizeof(command)-1) {  // buffer full: treat as a completed line
    command[sizeof(command)-2] = '\r';  // place end-of-line marker inside the buffer
    command[sizeof(command)-1] = 0;     // keep the buffer NUL-terminated
  }

  if (len > 0 && command[len - 1] == '\r') {  // received complete line
    Serial.print('\n');
    command[len - 1] = 0;  // replace newline with C string null terminator
    char reply[160];
    reply[0] = 0;
#ifdef ETHERNET_ENABLED
    if (!ethernet_handle_command(command, reply)) {
      the_mesh.handleCommand(0, command, reply);
    }
#else
    the_mesh.handleCommand(0, command, reply);  // NOTE: there is no sender_timestamp via serial!
#endif
    if (reply[0]) {
      Serial.print("  -> "); Serial.println(reply);
    }

    command[0] = 0;  // reset command buffer
  }

  board.loop();   // let the board feed its watchdog, run periodic housekeeping
#if defined(THINKNODE_M6) && ENV_INCLUDE_GPS == 1 && !defined(DISPLAY_CLASS)
  m6ButtonLoop();
#endif

#ifdef ETHERNET_ENABLED
  ethernet_loop_maintain();
  if (ethernet_read_line(ethernet_command, sizeof(ethernet_command))) {
    char reply[160];
    reply[0] = 0;
    if (!ethernet_handle_command(ethernet_command, reply)) {
      the_mesh.handleCommand(0, ethernet_command, reply);
    }
    ethernet_send_reply(reply);
    ethernet_command[0] = 0;
  }
#endif

#if defined(PIN_USER_BTN) && defined(_SEEED_SENSECAP_SOLAR_H_) && !defined(DISPLAY_CLASS)
  // Hold the user button to power off the SenseCAP Solar repeater.
  int btnState = digitalRead(PIN_USER_BTN);
  if (btnState == LOW) {
    if (userBtnDownAt == 0) {
      userBtnDownAt = millis();
    } else if ((unsigned long)(millis() - userBtnDownAt) >= USER_BTN_HOLD_OFF_MILLIS) {
      Serial.println("Powering off...");
      board.powerOff();  // does not return
    }
  } else {
    userBtnDownAt = 0;
  }
#endif

  the_mesh.loop();
  sensors.loop();
#ifdef DISPLAY_CLASS
  ui_task.loop();
#endif
  rtc_clock.tick();

#ifdef HAS_EXTERNAL_WATCHDOG
  external_watchdog.loop();
#endif
  if (the_mesh.getNodePrefs()->powersaving_enabled && !the_mesh.hasPendingWork()) {
#if defined(NRF52_PLATFORM)
    board.sleep(0); // nrf ignores seconds param, sleeps whenever possible
#else
    if (the_mesh.millisHasNowPassed(POWERSAVING_FIRSTSLEEP_SECS * 1000)) { // To check if it is time to sleep
      board.sleep(30); // Sleep. Wake up after a while or when receiving a LoRa packet
    }
#endif
  }
}
