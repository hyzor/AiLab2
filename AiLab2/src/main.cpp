#include <iostream>
#include <string>
#include <sstream>

#include "CrocGame.h"

#define NUM_GAMES 100
#define NUM_WATERHOLES 36
#define MOVE_SEARCH L"S"

struct GameState
{
	int score;
	int playerLocation;
	int backpacker1Activity;
	int backpacker2Activity;
	double calciumReading;
	double salineReading;
	double alkalinityReading;

	GameState() :
		score(0),
		playerLocation(0),
		backpacker1Activity(0),
		backpacker2Activity(0),
		calciumReading(0.0),
		salineReading(0.0),
		alkalinityReading(0.0)
	{}
};

//This is a comment, nope
double howClose(std::pair<double,double> distribution, double measurement)
{
	measurement = measurement - distribution.first; //distribution.first=mean
	if (measurement < 0)
	{
		measurement = -1*measurement;
	}
	measurement = measurement/distribution.second; //distribution.second=std
	return measurement;
}

int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	std::wstring groupName = L"Test";
	bool isOk = false;

	unsigned int numGamesFinished = 0;

	std::wcout << "Staring game using group name " << groupName.c_str() << std::endl;
	CrocSession* crocSession = new CrocSession(groupName, isOk);

	// Game paths
	std::vector<std::vector<int>> paths;
	paths = crocSession->getPaths(); // Paths are constant from game to game

	std::vector<int> gameScores;
	double waterHoleProbabilities[NUM_WATERHOLES] = { 0.0 };

	// The moves to be made
	std::wostringstream playerMove1Str;
	std::wostringstream playerMove2Str;

	// Play a certain amount of games before posting results
	while(numGamesFinished < NUM_GAMES)
	{
		crocSession->StartGame();

		bool gameIsRunning = true;

		// Game distributions
		// Distributions differ from game to game
		std::vector<std::pair<double,double>> calciumDist;
		std::vector<std::pair<double,double>> salinityDist;
		std::vector<std::pair<double,double>> alkalinityDist;

		crocSession->GetGameDistributions(
			calciumDist,
			salinityDist,
			alkalinityDist);

		// Game state
		GameState curGameState;

		// The amount of moves we have currently made
		int curGameScore = 0;

		// Clear our probabilities array
		std::fill_n(waterHoleProbabilities, NUM_WATERHOLES, 0.0);

		//--------------------------------------------
		// Game session loop
		//--------------------------------------------
		while (gameIsRunning)
		{
			// Get current game state
			crocSession->GetGameState(
				curGameState.score,
				curGameState.playerLocation,
				curGameState.backpacker1Activity,	// 0 if eaten (negative if being eaten)
				curGameState.backpacker2Activity,	// 0 if eaten (negative if being eaten)
				curGameState.calciumReading,		// Calcium reading at Croc location
				curGameState.salineReading,			// Saline reading at Croc location
				curGameState.alkalinityReading);	// Alkalinity reading at Croc location

			playerMove1Str.str(std::wstring());
			playerMove2Str.str(std::wstring());

			// The two moves to be made, assume player only searches for Croc
			// -1 means the player will search
			int playerMove1 = -1;
			int playerMove2 = -1;

			// Predict where the Croc is by estimating the probabilities for each waterhole
			// to have the Croc in it based on the readings from the Croc
			for (unsigned int i = 0; i < NUM_WATERHOLES; ++i)
			{

			}

			// A backpacker is currently being eaten, this reveals the Crocs location
			if (curGameState.backpacker1Activity < 0)
			{
				// Save this location and try to predict where the Croc will move next
				int crocLoc = curGameState.backpacker1Activity * -1;
			}
			else if (curGameState.backpacker2Activity < 0)
			{
				// Save this location and try to predict where the Croc will move next
				int crocLoc = curGameState.backpacker2Activity * -1;
			}

			if (playerMove1 == -1)
				playerMove1Str << MOVE_SEARCH;
			else
				playerMove1Str << playerMove1;

			if (playerMove2 == -1)
				playerMove2Str << MOVE_SEARCH;
			else
				playerMove2Str << playerMove2; 

			// After predicting the Crocs location and where the Croc is moving,
			// make moves (2 moves per 1 Croc/backpacker move)
			if (!crocSession->makeMove(playerMove1Str.str(), playerMove2Str.str(), curGameScore))
			{
				// Croc was found, game ends
				gameScores.push_back(curGameScore);
				numGamesFinished++;
				gameIsRunning = false;
			}
		}
	}

	// Post results after playing 100 games or more
	crocSession->PostResults();
	delete crocSession;

	return 0;
}
