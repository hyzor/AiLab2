#include <iostream>
#include <string>
#include <sstream>
#include <cmath>

#include "CrocGame.h"

#define NUM_GAMES 100
#define NUM_WATERHOLES 35
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

//Get a measure of how close the sensor data is to the given waterhole
//A greater result means a better match to the waterhole
double getInvertedDeviationFromMeasurement(std::vector<std::pair<double, double>> distributions, std::vector<double> readings)
{
	double deviation = 0;
	for(int i = 0; i < 3; i++)
		//Add |mean - measurement|/stdev
		deviation += std::abs(distributions[i].first - readings[i])/distributions[i].second;

	return 1/deviation;
}

std::vector<double> getEmissionProbabilities(double calciumRead,		
										     double salinityRead,		
											 double alkalinityRead,
										     std::vector<std::pair<double,double>> calciumDist,
											 std::vector<std::pair<double,double>> salinityDist,
										     std::vector<std::pair<double,double>> alkalinityDist){
	std::vector<double> emissionProbs(NUM_WATERHOLES, 0);
	std::vector<std::pair<double, double>> distributions(NUM_WATERHOLES, std::pair<double, double>(3, NULL));
	std::vector<double> measurements(3, 0);
	measurements[0] = calciumRead;
	measurements[1] = salinityRead;
	measurements[2] = alkalinityRead;

	double sumOfVector = 0;

	for(int i = 0; i < NUM_WATERHOLES; i++){
		distributions[0] = calciumDist[i];
		distributions[1] = salinityDist[i];
		distributions[2] = alkalinityDist[i];

		emissionProbs[i] = getInvertedDeviationFromMeasurement(distributions, measurements);
		sumOfVector += emissionProbs[i];
	}

	//Normalize the vector
	for(int i = 0; i < emissionProbs.size(); i++)
		emissionProbs[i] /= sumOfVector;

	return emissionProbs;
}

std::vector<double> viterbi(std::vector<double> V,
							std::vector<std::vector<double>> transProbs,
							std::vector<std::pair<double,double>> calciumDist, 
							std::vector<std::pair<double,double>> salinityDist, 
							std::vector<std::pair<double,double>> alkalinityDist,
							double calciumReading,
							double salineReading,
							double alkalinityReading,
							int knownLocation){
	std::vector<double> newV(NUM_WATERHOLES, 0);
	std::vector<double> emissionProbs(NUM_WATERHOLES, 0);
	if(knownLocation > -1) //Croc's location is known
		emissionProbs[knownLocation-1] = 1;
	else
		emissionProbs = getEmissionProbabilities(calciumReading, salineReading, alkalinityReading,
											     calciumDist, salinityDist, alkalinityDist);
	
	double prob;
	for(int i = 0; i < newV.size(); i++){
		for(int j = 0; j < calciumDist.size(); j++){
			prob = V[i]*transProbs[i][j]*emissionProbs[j];
			if(prob > newV[j])
				newV[j] = prob;
		}
	}

	double sum = 0;
	for(int i = 0; i < newV.size(); i++)
		sum += newV[i];

	for(int i = 0; i < newV.size(); i++)
		newV[i] /= sum;

	return newV;
}



int argMax(std::vector<double> v){
	double max = v[0];
	int index = 0;
	for(int i = 1; i < v.size(); i++){
		if(v[i] > max){
			max = v[i];
			index = i;
		}
	}
	return index;
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

	std::wcout << "Starting game using group name " << groupName.c_str() << std::endl;
	CrocSession* crocSession = new CrocSession(groupName, isOk);

	// Game paths
	std::vector<std::vector<int>> paths;
	paths = crocSession->getPaths(); // Paths are constant from game to game

	std::vector<int> gameScores;
	double waterHoleProbabilities[NUM_WATERHOLES] = { 0.0 };

	// The moves to be made
	std::wostringstream playerMove1Str;
	std::wostringstream playerMove2Str;

	//Transition probabilities used for Viterbi algorithm
	std::vector<std::vector<double>> transProbs (NUM_WATERHOLES, std::vector<double> (NUM_WATERHOLES, 0));;
	double iProbability; //Uniform transition probabilities from hole i
	for(int i = 0; i < paths.size(); i++){
		iProbability = (double)1/paths[i].size();
		for(int j = 0; j < paths[i].size(); j++){
			transProbs[i][paths[i][j]-1] = iProbability;
		}
	}

	//Probability that Croc is in a lake without any readings
	double initialProbability = (double)1/NUM_WATERHOLES;
			
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

		//Initialize the state vector V which will be used in the Viterbi algorithm
		std::vector<double> V (NUM_WATERHOLES, 0);

		//Get the game state 
		crocSession->GetGameState(
				curGameState.score,
				curGameState.playerLocation,
				curGameState.backpacker1Activity,	
				curGameState.backpacker2Activity,	
				curGameState.calciumReading,		
				curGameState.salineReading,			
				curGameState.alkalinityReading);

		//Emission probability vector
		//These probabilities represent how well Croc's readings matches each lake
		std::vector<double> emissionProbs(NUM_WATERHOLES, 0);
		emissionProbs = getEmissionProbabilities(curGameState.calciumReading,		
												 curGameState.salineReading,			
												 curGameState.alkalinityReading,
												 calciumDist,
												 salinityDist,
												 alkalinityDist);


		if(curGameState.backpacker1Activity < 0){
			V[curGameState.backpacker1Activity*-1 - 1] = 1;
		} else if(curGameState.backpacker2Activity < 0){
			V[curGameState.backpacker2Activity*-1 - 1] = 1;
		} else {
			for(int i = 0; i < NUM_WATERHOLES; i++)
				V[i] = initialProbability*emissionProbs[i];
		}

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

			//The waterhole to head for
			int crocLoc;

			// A backpacker is currently being eaten, this reveals the Crocs location
			if (curGameState.backpacker1Activity < 0)
			{
				// Save this location and try to predict where the Croc will move next
				crocLoc = curGameState.backpacker1Activity * -1;
				V = viterbi(V, transProbs, calciumDist, salinityDist, alkalinityDist,
					        curGameState.calciumReading, curGameState.salineReading,
							curGameState.alkalinityReading, curGameState.backpacker1Activity*-1);
			}
			else if (curGameState.backpacker2Activity < 0)
			{
				// Save this location and try to predict where the Croc will move next
				crocLoc = curGameState.backpacker2Activity * -1;
				V = viterbi(V, transProbs, calciumDist, salinityDist, alkalinityDist,
					        curGameState.calciumReading, curGameState.salineReading,
							curGameState.alkalinityReading, curGameState.backpacker2Activity*-1);
			} else {
				// Predict where the Croc is by estimating the probabilities for each waterhole
				// to have the Croc in it based on the readings from the Croc
				V = viterbi(V, transProbs, calciumDist, salinityDist, alkalinityDist,
					        curGameState.calciumReading, curGameState.salineReading,
							curGameState.alkalinityReading, -1);
				crocLoc = argMax(V)+1;
			}

			//The ranger must head for crocLoc and search if the waterhole is close enough
			std::vector<int> path = findShortestPath(curGameState.playerLocation, crocLoc, paths);
			if(path.size() > 1){
				playerMove1 = path[0];
				playerMove2 = path[1];
			} else if(path.size()>0){
				playerMove1 = path[0];
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
				std::cout << curGameScore << std::endl;
			}
		}
	}

	std::cout << "Game finished with an average of " << crocSession->getAverage();

	while(true){}
	// Post results after playing 100 games or more
	//crocSession->PostResults();
	delete crocSession;

	return 0;
}