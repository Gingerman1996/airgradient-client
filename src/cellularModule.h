/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#ifndef CELLULAR_MODULE_H
#define CELLULAR_MODULE_H

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

enum class CellReturnStatus {
  Ok = 1, // command is success and return expected value
  Failed, // command is success but not return expected value
  Error,  // module return error after command sent
  Timeout // module not return anything
};

template <typename T> struct CellResult {
  CellReturnStatus status;
  T data;
};

enum class CellTechnology { Auto, TWO_G, LTE_M, LTE_NB_IOT, LTE };

class CellularModule {
public:
  struct HttpResponse {
    int statusCode;
    std::unique_ptr<char[]> body;
    int bodyLen;
  };

  struct UdpPacket {
    std::vector<uint8_t> buff;
    int size;
  };

  // GNSS fix returned by AT+CGNSSINFO
  struct GnssFix {
    bool valid;             // true when latitude/longitude are populated
    double latitude;        // decimal degrees, +N / -S
    double longitude;       // decimal degrees, +E / -W
    float altitudeMeters;   // MSL altitude
    char dateUTC[7];        // "ddmmyy", null-terminated
    char timeUTC[10];       // "hhmmss.ss", null-terminated
  };

  // URL, Headers opt?, conn timeout, recv timeout,
  // response: CRS, status code, body

  CellularModule();
  virtual ~CellularModule();

  virtual bool init();
  virtual void powerOn();
  virtual void powerOff(bool force = false);
  virtual bool reset();
  virtual void sleep();
  virtual CellResult<std::string> getModuleInfo();
  virtual CellResult<std::string> retrieveSimCCID();
  virtual CellReturnStatus isSimReady();
  virtual CellResult<int> retrieveSignal();
  virtual CellResult<std::string> retrieveIPAddr();
  virtual CellResult<std::string> resolveDNS(const std::string &hostname);
  virtual bool setOperators(const std::string &serialized, uint32_t operatorId,
                            uint32_t registrationFailCount = 0);
  virtual std::string getSerializedOperators() const;
  virtual uint32_t getCurrentOperatorId() const;
  virtual uint32_t getRegistrationFailCount() const;
  virtual CellReturnStatus isNetworkRegistered(CellTechnology ct);
  virtual CellResult<std::string> startNetworkRegistration(CellTechnology ct,
                                                           const std::string &apn,
                                                           uint32_t operationTimeoutMs = 90000,
                                                           uint32_t scanTimeoutMs = 600000);
  virtual CellReturnStatus reinitialize();
  virtual CellResult<HttpResponse> httpGet(const std::string &url, int connectionTimeout = -1,
                                           int responseTimeout = -1);
  virtual CellResult<HttpResponse> httpPost(const std::string &url, const std::string &body,
                                            const std::string &headContentType = "",
                                            int connectionTimeout = -1, int responseTimeout = -1);
  virtual CellReturnStatus mqttConnect(const std::string &clientId, const std::string &host,
                                       int port = 1883, std::string username = "",
                                       std::string password = "");
  virtual CellReturnStatus mqttDisconnect();
  virtual CellReturnStatus mqttPublish(const std::string &topic, const std::string &payload,
                                       int qos = 1, int retain = 0, int timeoutS = 15);

  // GNSS subsystem control. Default base implementations are no-ops returning
  // failure; concrete modems with on-board GNSS (e.g. A76XX) override these.
  // Powers on GNSS and applies mode/NMEA configuration as per the working
  // feature/GNSS reference (see GNSS_Testing_Guide.md): CGNSSMODE=3 (GPS+QZSS),
  // CGNSSNMEA, CGPSNMEARATE=1.
  virtual bool gnssPowerOn(bool useHotStart = true, uint32_t readyTimeoutMs = 15000);
  virtual bool gnssPowerOff(bool saveHotStartCache = true);
  // Cold start: clear any cached almanac/ephemeris and re-acquire from scratch.
  virtual bool gnssColdStart();
  // Hot start: reuse cached data for fastest TTFF.
  virtual bool gnssHotStart();
  // Pull AGPS assistance data via cellular network (AT+CAGPS). Requires a
  // working data connection. Best-effort — non-fatal on failure.
  virtual bool gnssAgps();
  // Optional per-poll callback (e.g. to kick an external watchdog while waiting
  // for a fix). Invoked after every poll iteration (~1s).
  using GnssTickCb = std::function<void()>;
  virtual CellResult<GnssFix> gnssGetFix(uint32_t fixTimeoutMs = 90000,
                                         GnssTickCb onTick = nullptr);

  virtual CellReturnStatus udpConnect(const std::string &host, int port = 5683);
  virtual CellReturnStatus udpDisconnect();
  virtual CellReturnStatus udpSend(const UdpPacket &packet, const std::string &host, uint16_t port);
  virtual CellResult<UdpPacket> udpReceive(uint32_t timeout);

  // Generic functions

  /**
   * @brief convert cellular csq command (signal quality) to RSSI in dbm
   *
   * @param csq signal quality indicator to convert
   *
   * @return RSSI in dbm
   * @return 0 if signal quality indicator is invalid
   */
  int csqToDbm(int csq);

private:
  /* data */
};

#endif // CELLULAR_MODULE_H
