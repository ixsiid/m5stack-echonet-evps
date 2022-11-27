#pragma once
#include "udp-socket.hpp"

#pragma pack(1)

typedef struct {
	uint16_t _1081;
	uint16_t packet_id;
	uint16_t src_device_class;
	uint8_t src_device_id;
	uint16_t dst_device_class;
	uint8_t dst_device_id;
	uint8_t esv;
	uint8_t epc_count;
} elpacket_t;

#pragma pack()

#define EL_MULTICAST_IP "224.0.23.0"
#define EL_PORT 3610
#define EL_BUFFER_SIZE 256

// to HEMS用
class ELObject {
    protected:
	static uint8_t buffer[EL_BUFFER_SIZE + 32];
	static size_t buffer_length;
	static elpacket_t* p;
	static uint8_t* epc_start;

	static uint8_t* maker_code;

	static const uint16_t CLASS_HEMS = 0xff05;

	uint8_t _instance;
	uint16_t _class_id;

	virtual uint8_t get(uint8_t* epcs, uint8_t epc_count) = 0;
	virtual uint8_t set(uint8_t* epcs, uint8_t epc_count) = 0;

    public:
	ELObject();
	uint16_t class_id();
	uint8_t instance();
	int send(UDPSocket* udp, const esp_ip_addr_t* addr);

	bool process(const elpacket_t* recv, uint8_t* epc_array);
};

class Profile : public ELObject {
    private:
	uint8_t* profile[0xff];

	// uint16_t _class_id	 = 0x7e02;

	uint8_t get(uint8_t* epcs, uint8_t epc_count);
	uint8_t set(uint8_t* epcs, uint8_t epc_count);

    public:
	Profile();
};

enum class EVPS_Mode : uint8_t {
	Charge			 = 0x42,
	Discharge			 = 0x43,
	Stanby			 = 0x44,
	Charge_And_Discharge = 0x46,
	Stop				 = 0x47,
	Starting			 = 0x48,
	Auto				 = 0x49,
	Undefine			 = 0x40,
	Unacceptable         = 0x00,
};

typedef EVPS_Mode (*update_mode_cb_t)(EVPS_Mode current_mode, EVPS_Mode request_mode);

class EVPS : public ELObject {
    private:
	uint8_t* evps[0xff];

	uint8_t get(uint8_t* epcs, uint8_t epc_count);
	uint8_t set(uint8_t* epcs, uint8_t epc_count);

	update_mode_cb_t update_mode_cb;

    public:
	EVPS(uint8_t instance);
	void set_update_mode_cb(update_mode_cb_t cb);
	void notify_mode();
};
