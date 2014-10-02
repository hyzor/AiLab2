#include <iostream>

#include "CrocGame.h"

#define NUM_GAMES 100

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

//Return the index of the waterhole with the smallest G
int findMin(std::vector<int> open, std::vector<int> G){
	int min = G[open[0]-1];
	int index = 0;
	for(int i = 1; i < open.size(); i++){
		if(G[open[i]-1] < min){
			min = G[open[i]-1];
			index = i;
		}
	}
	return index;
}

bool isInVector(int waterhole, std::vector<int> theArray){
	for(int i = 0; i < theArray.size(); i++){
		if(theArray[i] == waterhole)
			return true;
	}
	return false;
}

//Trace back through parents to get a path
std::vector<int> traceBack(int current, int start, std::vector<int> parents){
	std::vector<int> temp; //Will store the path in the wrong direction
	int parent;
	while (current != start){ //As long as the start waterhole isn't reached...
		temp.push_back(current); //Add the waterhole to the path
		parent = parents[current-1]; //Get the parent
		current = parent; //Traverse to the parent waterhole
	}

	//Flip the vector
	std::vector<int> path;
	for(int i = temp.size()-1; i >= 0; i--)
		path.push_back(temp[i]);

	return path;
}

//Find the shortest path between two waterholes using Dijkstra's algorithm
//Note that the waterholes are zero EXCLUSIVE while indices are zero INCLUSIVE
std::vector<int> findShortestPath(int start, int goal, std::vector<std::vector<int>> paths){
	if(start == goal){
		std::vector<int> emptySet;
		return emptySet;
	}

	int size = 35;
	std::vector<int> G(size, 40);

	std::vector<int> open;
	std::vector<int> closed;
	std::vector<int> parent(size);

	open.push_back(start);
	G[start-1] = 0;

	int current; //The waterhole being expanded
	int neighbor; //Used to store the neighbor waterholes in the vector neighbors
	std::vector<int> neighbors; //Stores the current neighbors

	while(open.size() > 0)
	{
		//Expand the waterhole in the open set with lowest G
		int index = findMin(open, G);
		current = open[index];
		open.erase(open.begin()+index); //Remove from the open set...
		closed.push_back(current);      //... and add it to the closed set

		//If the waterhole is the goal waterhole we trace back to the start and return
		if (current == goal)
			return traceBack(current, start, parent);

		//Find neighbors
		neighbors.clear();
		neighbors = paths[current-1];

		int newGScore; //Holds G for neighbors

		//Explore all neighbor waterholes
		for(int i = 0; i < neighbors.size(); i++){
			if(!isInVector(neighbors[i], closed)){
				newGScore = G[current-1] + 1; //Distance from start through the current waterhole to the neighbor waterhole
				if(!isInVector(neighbors[i], open) || newGScore < G[neighbors[i]-1]) //Better path to this neighbor found
				{
					parent[neighbors[i]-1] = current;
					G[neighbors[i]-1] = newGScore;
					if(!isInVector(neighbors[i], open)) //If the neighbor waterhole isn't already in the open set it is added
						open.push_back(neighbors[i]);
				}
			}
		}
	}
	std::vector<int> epicFail;
	std::cout << "Dijkstra has failed!" << std::endl;
	return epicFail; //Hopefully this doesn't happen
}

//Method for testing the Dijkstra algorithm
void testDijkstra(int start, int goal, std::vector<std::vector<int>> paths){
	std::cout << "***Trying Dijkstra's algorithm from hole " << start << " to hole " << goal << "***";
	
	std::vector<int> path = findShortestPath(start, goal, paths);
	std::cout << std::endl << std::endl << "Best found path: " << std::endl;
	for(int i = 0; i < path.size(); i++){
		std::cout << path[i] << " ";
	}
	std::cout << std::endl << std::endl;

	std::cout << "Paths: " << std::endl;
	for(int i = 0; i < paths.size(); i++){
		std::cout << i+1 << ": ";
		for(int j = 0; j < paths[i].size(); j++){
			std::cout << paths[i][j] << " ";
		}
		std::cout << std::endl;
	}

	while(true){}
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

			// The two moves to be made, assume player only searches for Croc
			std::wstring playerMove = L"S";
			std::wstring playerMove2 = L"S";
			int outScore;

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

			// After predicting the Crocs location and where the Croc is moving,
			// make moves (2 moves per 1 Croc/backpacker move)
			if (!crocSession->makeMove(playerMove, playerMove2, outScore))
			{
				// Croc was found, game ends
				gameIsRunning = false;
				numGamesFinished++;
			}
		}
	}

	// Post results after playing 100 games or more
	crocSession->PostResults();
	delete crocSession;

	return 0;
}