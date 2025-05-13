#include "RFComm.hpp"

RF69_Comm::RF69_Comm(uint8_t node_id, float frequency) 
  : _radio(RFM69_CS, RFM69_INT), _node_id(node_id), _frequency(frequency) {}

bool RF69_Comm::begin() {
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);
  delay(10);
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);

  if (!_radio.init()) return false;
  if (!_radio.setFrequency(_frequency)) return false;
  _radio.setTxPower(20, true);
  return true;
}

void RF69_Comm::send(uint8_t receiver_id, uint8_t command, const char* message) {
  RF69_Packet packet;
  packet.sender_id = _node_id;
  packet.receiver_id = receiver_id;
  packet.command = command;
  strncpy(packet.payload, message, sizeof(packet.payload)-1);
  
  // Calculate CRC over header + payload
  uint8_t crc_data[sizeof(packet)-1];
  memcpy(crc_data, &packet, sizeof(crc_data));
  packet.crc = _calculate_crc(crc_data, sizeof(crc_data));
  
  _radio.send((uint8_t*)&packet, sizeof(packet));
  _radio.waitPacketSent();
}

void RF69_Comm::update() {
  if (_radio.available()) {
    RF69_Packet packet;
    uint8_t len = sizeof(packet);
    if (_radio.recv((uint8_t*)&packet, &len)) {
      // Verify CRC
      uint8_t calculated_crc = _calculate_crc((uint8_t*)&packet, len-1);
      if(packet.crc == calculated_crc && 
         (packet.receiver_id == _node_id || packet.receiver_id == 0xFF)) {
        if(_receive_handler) _receive_handler(packet);
      }
    }
  }
}

uint8_t RF69_Comm::_calculate_crc(const uint8_t* data, uint8_t len) {
  uint8_t crc = 0;
  for(uint8_t i=0; i<len; i++) crc ^= data[i];
  return crc;
}

void RF69_Comm::set_receive_handler(void (*handler)(RF69_Packet &packet)) {
  _receive_handler = handler;
}