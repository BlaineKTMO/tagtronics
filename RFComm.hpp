#ifndef RF_COMM_HPP
#define RF_COMM_HPP

#include <RH_RF69.h>
#include <SPI.h>

// Platform detection for Feather boards
#if defined(ADAFRUIT_FEATHER_M0) || defined(ADAFRUIT_FEATHER_M0_EXPRESS)
  #define RFM69_CS    8
  #define RFM69_INT   3
  #define RFM69_RST   4
#else
  #error "Unsupported board - add pin definitions for your platform"
#endif

struct RF69_Packet {
  uint8_t sender_id;
  uint8_t receiver_id;
  uint8_t command;
  char payload[RH_RF69_MAX_MESSAGE_LEN - 3]; // Reserve 3 bytes for headers
  uint8_t crc;
};

class RF69_Comm {
  public:
    RF69_Comm(uint8_t node_id, float frequency = 868.0);
    bool begin();
    void send(uint8_t receiver_id, uint8_t command, const char* message);
    void update();
    void set_receive_handler(void (*handler)(RF69_Packet &packet));

  private:
    RH_RF69 _radio;
    uint8_t _node_id;
    float _frequency;
    void (*_receive_handler)(RF69_Packet &);
    uint8_t _calculate_crc(const uint8_t* data, uint8_t len);
};

#endif // RF_COMM_HPP