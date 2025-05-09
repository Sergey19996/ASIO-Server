#include <iostream>
#include <olc_net.h>

enum class CustomMsgTypes : uint32_t {
	FireBullet,
	MovePlayer

};

class CustomClient : public olc::net::client_interface<CustomMsgTypes>
{
public:
	bool FireBullet(float x, float y)
	{
		olc::net::message<CustomMsgTypes> msg;
		msg.header.id = CustomMsgTypes::FireBullet;
		msg << x << y;
		Send(msg);
	}
};

int main() {
	CustomClient c;
	c.Connect("community.onelonecider.com", 60000);
	c.FireBullet(2.0f, 5.0f);

	//olc::net::message<CustomMsgTypes> msg;
	//msg.header.id = CustomMsgTypes::FireBullet;  // enum int (0)

	//int a = 1;
	//bool b = true;
	//float c = 3.14159f;

	//struct
	//{
	//	float x;
	//	float y;

	//}d[5];

	//msg << a << b << c << d;

	//a = 99;
	//b = false;
	//c = 99.0f;

	//msg >> d >> c >> b >> a;



	return 0;

}