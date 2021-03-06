///@file
// simple IR signal detector
#include "hwlib.hpp"
#include "transmitter.hpp"
#include "gameParameters.hpp"
#include "messageLogic.hpp"
#include "receiverController.hpp"
#include "InitGameController.hpp"
#include "KeypadController.hpp"
#include "RunGameController.hpp"
#include "SpeakerController.hpp"
#include "GameParamsController.hpp"

#include "OLEDBoundary.hpp"

#include "hwlib.hpp"
#include "rtos.hpp"
#include <array>

int main( void ){
   	// kill the watchdog
  	WDT->WDT_MR = WDT_MR_WDDIS;

	namespace target = hwlib::target;
   	//Wait for hell freezing over
	hwlib::wait_ms(1000);

   	//Set button and led pins
	auto button = hwlib::target::pin_in(hwlib::target::pins::d22);
	auto led = hwlib::target::pin_out(hwlib::target::pins::d24);

	//Set IR receiver pins
    auto vcc = hwlib::target::pin_out(hwlib::target::pins::d10);
    auto gnd = hwlib::target::pin_out(hwlib::target::pins::d9);
    auto data = hwlib::target::pin_in(hwlib::target::pins::d8);

    //Set speaker pin
    auto lsp = target::pin_out( target::pins::d7);

    //Set keypad pins
    auto out0 = target::pin_oc( target::pins::a0 );
	auto out1 = target::pin_oc( target::pins::a1 );
	auto out2 = target::pin_oc( target::pins::a2 );
	auto out3 = target::pin_oc( target::pins::a3 );

	auto in0  = target::pin_in( target::pins::a4 );
	auto in1  = target::pin_in( target::pins::a5 );
	auto in2  = target::pin_in( target::pins::a6 );
	auto in3  = target::pin_in( target::pins::a7 );

	auto out_port = hwlib::port_oc_from_pins( out0, out1, out2, out3 );
	auto in_port  = hwlib::port_in_from_pins( in0,  in1,  in2,  in3  );
	auto matrix   = hwlib::matrix_of_switches( out_port, in_port );
	auto keypad   = hwlib::keypad< 16 >( matrix, "123A456B789C*0#D" );

	//Create a test message and encode it
	messageLogic messageLogic;
    char16_t compiledMessage = messageLogic.encode(1,1);

	//Set the playerInformation according to the message
    playerInformation playerInfo;
    playerInfo.setCompiledBits(compiledMessage);

	//Set receivercontroller
    receiverController receiver{ data, gnd, vcc, messageLogic, 0 };
	transmitterController transmitter{ 1 };

	irentity IRE(button, led, transmitter, messageLogic, receiver);

	KeypadController kpC = KeypadController(keypad, 15);
	OLEDBoundary oledBoundary{ 17 };
	auto sC = SpeakerController(lsp, 14);
	auto rGC = RunGameController(kpC, sC, oledBoundary, playerInfo, IRE, 19);
	auto iGC = InitGameController(kpC, rGC, oledBoundary, playerInfo, IRE, 13);
	auto gPC = GameParamsController(kpC, iGC, rGC, oledBoundary, playerInfo, IRE, 16);

	kpC.registerNext(&gPC);

	// set IR receiver
	receiver.setReceiveListener(&rGC);

	rtos::run();
}
