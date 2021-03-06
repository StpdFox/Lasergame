///	\file InitgameController.cpp
/// The InitgameController file,
/// contains the InitgameController class implementation only.
/// Date file created:
/// \date 07-07-2017
/// Date Last Update:
/// \date 14-07-2017

#include "InitGameController.hpp"
#include "KeypadListener.hpp"
#include "KeypadController.hpp"
#include "OLEDBoundary.hpp"
#include "transmitterController.hpp"
#include "messageLogic.hpp"
#include "RunGameController.hpp"

InitGameController::InitGameController(KeypadController& kpC, RunGameController& runGameController, OLEDBoundary& oledBoundary, playerInformation& playerInfo, irentity& irEntity, unsigned int priority) :
    task(priority, "initGame task"),
	state{ STATE::WAITING_FOR_C },
	irEntity{ irEntity },
	playerInfo{ playerInfo },
	oledBoundary{ oledBoundary },
	statusStream{ oledBoundary.getStatusMessageField(), font },
	confirmStream{ oledBoundary.getConfirmMessageField(), font },
	timeStream{ oledBoundary.getGameDurationInputField(), font },
	keypadController{kpC},
	msg("keypad char"),
	runGameController{runGameController},
	startFlag{ this, "startFlag" }
{ }

void InitGameController::handleMessageKey(char c)  {
   msg.write(c);
}

void InitGameController::main()
{
	wait(startFlag);
	oledBoundary.getStatusMessageField().setLocation({ 4 * 8, 1 * 8 });
	oledBoundary.getConfirmMessageField().setLocation({ 2 * 8, 5 * 8 });
	oledBoundary.getGameDurationInputField().setLocation({ 4 * 8, 4 * 8 });
	keypadController.registerNext(this);
	statusStream << "\fYou are\nHost";
	confirmStream << "\f C to setup";
	oledBoundary.flushParts();
	for(;;) {
		char c = msg.read();
		KeyConsumer::handleMessageKey(*this, c);
	}
}

void InitGameController::initNewCommand() {
	commandCode[0] = 0;
	commandCode[1] = 0;
	commandCount = 0;
	statusStream << "\fInput time";
	timeStream << "\f_";
	oledBoundary.flushParts();
}

bool InitGameController::validateCommand() {
	if(    commandCode[0] >= '0' && commandCode[0] <= '9'
		&& commandCode[1] >= '0' && commandCode[1] <= '9')  {
		byte command = (commandCode[0] - '0') * 10 + (commandCode[1] - '0');
		return command >= 1 && command <= 15;
	}
	return false;
}

void InitGameController::sendMessage() {
	// Send the message over the air with apple update system
	HWLIB_TRACE << "InitGameController sendMessage";
	irEntity.receive.suspend();
	irEntity.trans.sendMessage(playerInfo.getCompiledBits());
	hwlib::wait_ms(1000);
	irEntity.receive.resume();
}

void InitGameController::sendStartMessage() {
	// Send the start message + start timer over the air with apple update system
	// Deregisters self and registers other keypadlistener
	HWLIB_TRACE << "InitGameController sendStartMessage";
	irEntity.receive.suspend();
	irEntity.trans.sendMessage(playerInfo.getCompiledBits());
	hwlib::wait_ms(1000);
	irEntity.receive.resume();
}

// Keypad Methods
void InitGameController::consumeChar(char c) {
	if(state == STATE::WAITING_FOR_C && c == 'C')
	{
		confirmStream << "\f";
		oledBoundary.flushParts();
		initNewCommand();
		HWLIB_TRACE << "state = STATE::INPUTTING_CMD";
		state = STATE::INPUTTING_CMD;
	}
	else if(state == STATE::SENDING_START_CMD && c == 'C')
	{
		hwlib::cout << "To runGame";
		runGameController.startGame(gameDurationMin);
		suspend();
	}
}

void InitGameController::consumeHashTag() {
	// If the command has been validated, it can be executed
	if(state == STATE::WAITING_FOR_HASHTAG)
	{
		if(validateCommand()) {
			timeStream << "\f";
			statusStream << "\f# to send\nthe cmd";
			confirmStream << "\f* to start\nothers";
			HWLIB_TRACE << "command is valid";
			gameDurationMin = (commandCode[0] - '0') * 10 + (commandCode[1] - '0');
			char16_t timeBits = irEntity.logic.encode(0, gameDurationMin);
			playerInfo.setCompiledBits(timeBits); //playerInformation is the object in the transmitterController
			HWLIB_TRACE << hwlib::bin << playerInfo.getCompiledBits() << "\n";
			HWLIB_TRACE << "state = STATE::SENDING_CMD";
			state = STATE::SENDING_CMD;
		} else {
			HWLIB_TRACE << "command is not valid";
			statusStream << "\finvalid";
			initNewCommand();
			HWLIB_TRACE << "state = STATE::INPUTTING_CMD";
			state = STATE::INPUTTING_CMD;
		}
		oledBoundary.flushParts();
	}
	else if(state == STATE::SENDING_CMD)
	{
		sendMessage();
	}
}

void InitGameController::consumeWildcard() {
	// Send a start message
	if(state == STATE::SENDING_CMD)
	{
		statusStream << "\fC to start\nGame";
		timeStream << "\f";
		oledBoundary.flushParts();

		char16_t commandBits = irEntity.logic.encode(0, 0);
		playerInfo.setCompiledBits(commandBits);
		HWLIB_TRACE << "state = STATE::SENDING_START_CMD";
		state = STATE::SENDING_START_CMD;
	}
	if(state == STATE::SENDING_START_CMD)
	{
		sendStartMessage();
	}
}

void InitGameController::consumeDigits(char c) {
	if(state == STATE::INPUTTING_CMD)
	{
		HWLIB_TRACE << c << " pressed";
		commandCode[commandCount++] = c;
		if(commandCount == 1)
		{
			timeStream << "\f" << c << "_";
		}
		else if(commandCount == 2)
		{
			timeStream << "\f" << commandCode[0] << c;
			confirmStream << "\f\n# to confirm";

			HWLIB_TRACE << "state = STATE::WAITING_FOR_HASHTAG";
			state = STATE::WAITING_FOR_HASHTAG;
		}
		oledBoundary.flushParts();
	}
}
void InitGameController::start()
{
	startFlag.set();
}
