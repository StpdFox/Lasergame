#define GAMETIME_WIDTH 5
#define GAMETIME_HEIGHT 1

#include "RunGameController.hpp"
#include "OLEDBoundary.hpp"

RunGameController::RunGameController(KeypadController& kpC, ISound& sound, OLEDBoundary& oledBoundary, unsigned int priority ) :
	rtos::task<>{ priority, "RunGameController" },
	kpC{kpC}, sound{sound}, 
	oledBoundary{ oledBoundary },
	registerFlag(this, "registerFlag"),
	gameTimeSecondsClock{ this, 1 * rtos::s, "gameTimeSecondsClock" }
	
	//gameTimeStream{ oledBoundary.getGameTimeField(), hwlib::font_default_8x8() }
{
	oledBoundary.getGameTimeField().setLocation({ 7 * 8, 6 * 8 });
}

RunGameController::~RunGameController()
{
}

void RunGameController::main()
{
	hwlib::glcd_oled_buffered& lcd = oledBoundary.getBufferedLCD();
	auto f = hwlib::font_default_8x8();
	auto stream = hwlib::window_ostream{ lcd, f };
	stream << "\f";
	oledBoundary.flush();
	
	startOfGameTimestamp = hwlib::now_us();
	gameDurationMin = 1;
	
	hwlib::window_ostream gameTimeStream{ oledBoundary.getGameTimeField(), f };
	while(true)
	{	
		const rtos::event& event = wait();
		  if(event == registerFlag)   {
			 kpC.registerNext(this);
		  }
		else if(event == gameTimeSecondsClock)
		{
			int remainingTimeSec = gameDurationMin * 60 - (hwlib::now_us() - startOfGameTimestamp) / 1'000'000;
			gameTimeStream << "\f" << remainingTimeSec / 60 << ":" << remainingTimeSec % 60;
			oledBoundary.flushParts();
			
			if(remainingTimeSec <= 0)
			{
				HWLIB_TRACE << "Game over!";
				while(true) sleep(1);
			}
		}
	}
}

void RunGameController::handleMessageKey(char c)  {
    hwlib::cout << c << " was written in testTask \n";
	switch(c)	{
		case '#':
			// Speakercontroller set flag for sound
			sound.setSound(Sounds::HIT);
		break;
	}
}