#include <math.h>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/filesystem.hpp>   
#include <boost/filesystem/fstream.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include "Utils.hpp"
#include "Identifier.h"
#include "Config.h"
#include "Logger.h"

using namespace boost::filesystem;
using namespace std;
namespace nsBFS = boost::filesystem;

typedef boost::gregorian::date Date;
typedef string TeamId;

struct Game{
    Date                      mDate;
    TeamId                    mHomeTeam;
    TeamId                    mAwayTeam;
    size_t                    mHomeGoals;
    size_t                    mAwayGoals;
    double                    mHomeOdds;
    double                    mDrawOdds;
    double                    mAwayOdds;
    double                    mHomeSharpe;
    double                    mDrawSharpe;
    double                    mAwaySharpe;
};

struct Model{
    map<TeamId, double> a;
    map<TeamId, double> d;
    double              h;
};

void
loadStats(vector<Game>& rAllGames, vector<Game>& rTrainingGames,
          vector<Game>& rFutureGames)
{
    string lastDateString = confParam<string>("last_date");
    Date lastDate = boost::gregorian::from_simple_string(lastDateString);

    string statsFileName = confParam<string>("files.stats");
    std::ifstream statsFile(statsFileName.c_str());
    
    if (!statsFile.is_open()){
        LOG_ERROR << "Could not open stats file " << statsFileName;
        return;
    }

    size_t numLine = 0;
    while (!statsFile.eof()){
        string line;
        getline(statsFile, line);
        ++numLine;

        vector<string> splitLine;
        splitString(line,',',splitLine);
        if (!line.size())
            continue;

        if (splitLine.size() != 5){
            LOG_WARNING << "Error reading stats file on line " << numLine
                        << ": " << line;
            continue;
        }
        Game game;
        game.mDate = boost::gregorian::from_simple_string(splitLine[0]);
        game.mHomeTeam = splitLine[1];
        game.mAwayTeam = splitLine[2];
        game.mHomeGoals = fromString<size_t>(splitLine[3]);
        game.mAwayGoals = fromString<size_t>(splitLine[4]);
        game.mHomeOdds = game.mDrawOdds = game.mAwayOdds = 0.0;
        LOG_TRACE << "Read line " << numLine << ": "
                  << boost::gregorian::to_simple_string(game.mDate) << "," 
                  << game.mHomeTeam << "," << game.mAwayTeam << ","
                  << game.mHomeGoals << "," << game.mAwayGoals;
        rAllGames.push_back(game);
        if (game.mDate <= lastDate)
            rTrainingGames.push_back(game);
        else
            rFutureGames.push_back(game);
    }
    LOG_INFO << "Loaded " << rAllGames.size() << " games, of which "
             << rTrainingGames.size() << " training games";
}

void
loadBets(vector<Game>& rBetGames)
{
    string betsFileName = confParam<string>("files.bets");
    std::ifstream betsFile(betsFileName.c_str());
    
    if (!betsFile.is_open()){
        LOG_ERROR << "Could not open stats file " << betsFileName;
        return;
    }

    size_t numLine = 0;
    while (!betsFile.eof()){
        string line;
        getline(betsFile, line);
        ++numLine;

        vector<string> splitLine;
        splitString(line,',',splitLine);
        if (!line.size())
            continue;

        if (splitLine.size() != 6){
            LOG_WARNING << "Error reading bets file on line " << numLine
                        << ": " << line;
            continue;
        }
        Game game;
        game.mDate = boost::gregorian::from_simple_string(splitLine[0]);
        game.mHomeTeam = splitLine[1];
        game.mAwayTeam = splitLine[2];
        game.mHomeGoals = -1;
        game.mAwayGoals = -1;
        game.mHomeOdds = fromString<double>(splitLine[3]);
        game.mDrawOdds = fromString<double>(splitLine[4]);
        game.mAwayOdds = fromString<double>(splitLine[5]);
        LOG_TRACE << "Read line " << numLine << ": "
                  << boost::gregorian::to_simple_string(game.mDate) << "," 
                  << game.mHomeTeam << "," << game.mAwayTeam << ","
                  << game.mHomeGoals << "," << game.mAwayGoals;
        rBetGames.push_back(game);
    }
    LOG_INFO << "Loaded " << rBetGames.size() << " bet games";

}


void
improveAs(const vector<Game>& games, Model& rModel)
{
    map<TeamId, double> sumMs;
    map<TeamId, double> sumHDs;
    for (size_t i = 0; i < games.size(); ++i){
        sumMs[games[i].mHomeTeam] += games[i].mHomeGoals;
        sumHDs[games[i].mHomeTeam] += rModel.d[games[i].mAwayTeam] * rModel.h;

        sumMs[games[i].mAwayTeam] += games[i].mAwayGoals;
        sumHDs[games[i].mAwayTeam] += rModel.d[games[i].mHomeTeam];
    }
    for (map<TeamId, double>::iterator it = rModel.a.begin();
         it != rModel.a.end(); ++it)
    {
        it->second = sumMs[it->first] / sumHDs[it->first];
    }
}

void
improveDs(const vector<Game>& games, Model& rModel)
{
    map<TeamId, double> sumMs;
    map<TeamId, double> sumHAs;
    for (size_t i = 0; i < games.size(); ++i){
        sumMs[games[i].mHomeTeam] += games[i].mAwayGoals;
        sumHAs[games[i].mHomeTeam] += rModel.a[games[i].mAwayTeam];

        sumMs[games[i].mAwayTeam] += games[i].mHomeGoals;
        sumHAs[games[i].mAwayTeam] += rModel.a[games[i].mHomeTeam] * rModel.h;
    }
    for (map<TeamId, double>::iterator it = rModel.d.begin();
         it != rModel.d.end(); ++it)
    {
        it->second = sumMs[it->first] / sumHAs[it->first];
    }
}

void
improveH(const vector<Game>& games, Model& rModel)
{
    double sumMs = 0.0;
    double sumADs = 0.0;
    for (size_t i = 0; i < games.size(); ++i){
        sumMs += games[i].mHomeGoals;
        sumADs += rModel.a[games[i].mHomeTeam] * rModel.d[games[i].mAwayTeam];

    }
    rModel.h = sumMs / sumADs;
}

void
initializeModel(const vector<Game>& games, Model& rModel)
{
    rModel.h = 1.0;
    for (size_t i = 0; i < games.size(); ++i){
        rModel.a[games[i].mHomeTeam] = 1.0;
        rModel.d[games[i].mHomeTeam] = 1.0;
        rModel.a[games[i].mAwayTeam] = 1.0;
        rModel.d[games[i].mAwayTeam] = 1.0;
    }

    string dbgStr;
    for (map<TeamId, double>::iterator it = rModel.d.begin();
         it != rModel.d.end(); ++it)
    {
        dbgStr += it->first + "           ";
    }
    LOG_INFO << dbgStr;
}

void
calculateVariability(const vector<Game>& games, Model& rModel)
{
    double dataVar = 0.0;
    double modelVar = 0.0;
    for (size_t i = 0; i < games.size(); ++i){
        double m = games[i].mHomeGoals;
        double mHat = rModel.h * 
            rModel.a[games[i].mHomeTeam] * rModel.d[games[i].mAwayTeam];
        modelVar += mHat;
        dataVar += (m - mHat) * (m - mHat);

        double n = games[i].mAwayGoals;
        double nHat = 
            rModel.a[games[i].mAwayTeam] * rModel.d[games[i].mHomeTeam];
        modelVar += nHat;
        dataVar += (n - nHat) * (n - nHat);
    }
    dataVar *= 1.0 * games.size() * 2 / 
        (games.size() * 2 - rModel.a.size() - rModel.d.size() - 1);
    LOG_INFO << "data var: " << dataVar << " of which " << modelVar 
             << "(" << modelVar / dataVar * 100 << "%) model var"; 
}

void
calculateModel(const vector<Game>& games, Model& rModel)
{
    initializeModel(games, rModel);

    size_t maxIter = confParam<size_t>("max_iterations");
    for (size_t iIter = 0; iIter < maxIter; ++iIter){
        calculateVariability(games, rModel);
        improveAs(games, rModel);
        improveDs(games, rModel);
        improveH(games,rModel);
        ostringstream dbgStr;
        for (map<TeamId, double>::iterator it = rModel.a.begin();
             it != rModel.a.end(); ++it)
        {
            dbgStr << left << setprecision(3) << setw(5) << it->second << " "
                   << setprecision(3) << setw(5) << rModel.d[it->first] 
                   << "   ";
        }
        LOG_INFO << " " << dbgStr.str() << ": h=" << rModel.h;
    }
}

double
poissonProb(size_t m, double lambda)
{
    return exp(-lambda + m * log(lambda) - lgamma(m + 1));
}

void
calcProbs(double lambdaHome, double lambdaAway,
          double& rHome, double& rDraw, double& rAway)
{
    static const double bracket = confParam<double>("algo.bracket");
    static const double tiedBias = confParam<double>("algo.tied_bias");

    static size_t maxScore = confParam<size_t>("max_score");
    rHome = rDraw = rAway = 0.0;
    for (size_t iHome = 0; iHome < maxScore; ++iHome)
        for (size_t iAway = 0; iAway < maxScore; ++iAway){
            double prob = poissonProb(iHome, lambdaHome) * 
                poissonProb(iAway, lambdaAway);
            if (iAway < iHome)
                rHome += prob;
            else if (iHome < iAway)
                rAway += prob;
            else
                rDraw += prob;
        }
    rDraw *= tiedBias;
    double sum = rDraw + rHome + rAway;
    rDraw /= sum;
    rHome /= sum;
    rAway /= sum;

    rHome *= bracket;
    rDraw *= bracket;
    rAway *= bracket;
}

void
calculateLambdas(const Game& game, const Model& model, 
                 double& rLambdaHome, double& rLambdaAway)
{
        TeamId homeT = game.mHomeTeam;
        double homeA = model.a.find(homeT)->second;
        double homeD = model.d.find(homeT)->second;

        TeamId awayT = game.mAwayTeam;
        double awayA = model.a.find(awayT)->second;
        double awayD = model.d.find(awayT)->second;

        rLambdaHome = model.h * homeA * awayD;
        rLambdaAway = awayA * homeD;
}

void
testModel(const vector<Game>& games, const Model& model)
{
    static const double bracket = confParam<double>("algo.bracket");

    double numHome = 0.0;
    double numDraw = 0.0;
    double numAway = 0.0;
    double predHome = 0.0;
    double predDraw = 0.0;
    double predAway = 0.0;
    double stdHome = 0.0;
    double stdDraw = 0.0;
    double stdAway = 0.0;
    
    double goalProduct = 0.0;
    double predGoalProduct = 0.0;

    for (size_t i = 0; i < games.size(); ++i){
        double lambdaHome, lambdaAway;
        calculateLambdas(games[i], model, lambdaHome, lambdaAway);

        double homeWin = 0.0;
        double draw = 0.0;
        double awayWin = 0.0;
        calcProbs(lambdaHome, lambdaAway, homeWin, draw, awayWin);
        homeWin /= bracket;
        draw /= bracket;
        awayWin /= bracket;

        predHome += homeWin;
        predDraw += draw;
        predAway += awayWin;
        stdHome += homeWin * (1.0 - homeWin);
        stdDraw += draw * (1.0 - draw);
        stdAway += awayWin * (1.0 - awayWin);

        goalProduct += games[i].mHomeGoals * games[i].mAwayGoals;
        predGoalProduct += lambdaHome * lambdaAway;

        if (games[i].mHomeGoals > games[i].mAwayGoals)
            ++numHome;
        else if (games[i].mHomeGoals < games[i].mAwayGoals)
            ++numAway;
        else
            ++numDraw;
        
    }
    stdHome = sqrt(stdHome);
    stdDraw = sqrt(stdDraw);
    stdAway = sqrt(stdAway);
    
    string testFileName = confParam<string>("files.model_test");
    std::ofstream testFile(testFileName.c_str());
    
    if (!testFile.is_open()){
        LOG_ERROR << "Could not open stats file " << testFileName;
        return;
    }

    testFile << "Home: " << setprecision(3) << predHome << " +- " 
             << setprecision(3) << stdHome << "   " << numHome << endl; 
    testFile << "Draw: " << setprecision(3) << predDraw << " +- " 
             << setprecision(3) << stdDraw << "   " << numDraw << endl; 
    testFile << "Away: " << setprecision(3) << predAway << " +- " 
             << setprecision(3) << stdAway << "   " << numAway << endl; 
    testFile << "Goal Product: " << setprecision(3) << predGoalProduct 
             << " +- " << setprecision(3) << "0" 
             << "   " << goalProduct << endl; 
    
}


double 
sharpe(double p, double b) //p - probability of winning, b - odds
{
    if (b == 0.0)
        return -1.0;
    return (b * p - 1) / b / sqrt(p * (1 - p));
}

template<class T>
struct ResultVec{
    
    T mHome;
    T mDraw;
    T mAway;

    const T& operator[](const size_t& i) const{
        if (i == 0)
            return mHome;
        else if (i == 1)
            return mDraw;
        else if (i == 2)
            return mAway;
        else{
            assert(0);
            return mHome;
        }
    }
    T& operator[](const size_t& i){
        if (i == 0)
            return mHome;
        else if (i == 1)
            return mDraw;
        else if (i == 2)
            return mAway;
        else{
            assert(0);
            return mHome;
        }
    }
};

struct PredictionData{
    ResultVec<double> mProb;
    ResultVec<double> mOdds;
    ResultVec<double> mMu;
    ResultVec<double> mV;
    ResultVec<double> mSharpe;
    double            mStake;
    
    size_t bestBet() const {
        if (mSharpe[0] > mSharpe[1] && mSharpe[0] > mSharpe[2])
            return 0;
        else if (mSharpe[1] > mSharpe[0] && mSharpe[1] > mSharpe[2])
            return 1;
        else
            return 2;
    }

    void
    calculateStats(){
        for (size_t i = 0; i < 3; ++i){
            mMu[i] = max(mOdds[i] * mProb[i] - 1.0, 0.0);

            mV[i] = pow(mOdds[i], 2) * mProb[i] * (1.0 - mProb[i]); 
            mSharpe[i] = mMu[i] / sqrt(mV[i]);
        }
    }
    
            
};

string
indexToBet(size_t i)
{
    if (i == 0)
        return "home";
    if (i == 1)
        return "draw";
    if (i == 2)
        return "away";
    else
        return "error";
}

void
predictFutureScores(const vector<Game>& games, const Model& model, 
                    const string& fileName)
{
    std::ofstream predFile(fileName.c_str());
    if (!predFile){
        LOG_ERROR << "Could not open " << fileName;
        return;
    }
    predFile << "Home factor: " << model.h << endl;
    
    for (size_t i = 0; i < games.size(); ++i){
        double lambdaHome, lambdaAway;
        calculateLambdas(games[i], model, lambdaHome, lambdaAway);

        TeamId homeT = games[i].mHomeTeam;
        double homeA = model.a.find(homeT)->second;
        double homeD = model.d.find(homeT)->second;

        TeamId awayT = games[i].mAwayTeam;
        double awayA = model.a.find(awayT)->second;
        double awayD = model.d.find(awayT)->second;


        predFile << homeT << " vs. " << awayT << endl;
        predFile << "    " 
                 << homeT 
                 << ": a=" << homeA 
                 << " d=" << homeD << endl;
        predFile << "    " 
                 << awayT 
                 << ": a=" << awayA 
                 << " d=" << awayD << endl;
        predFile << "    " << "expected score: " 
                 <<  lambdaHome << " " << lambdaAway << endl;
        predFile << "    " << "actual score: " 
                 <<  games[i].mHomeGoals << " " << games[i].mAwayGoals << endl;
        double homeWin = 0.0;
        double draw = 0.0;
        double awayWin = 0.0;
        calcProbs(lambdaHome, lambdaAway, homeWin, draw, awayWin);

        predFile << "                    " 
                 << setw(10) << "home" 
                 << setw(10) << "draw"  
                 << setw(10) << "away" << endl;  
        predFile << "calculated odds:    " 
                 << setw(10) << setprecision(3) << 1.0 / homeWin 
                 << setw(10) << setprecision(3) << 1.0 / draw  
                 << setw(10) << setprecision(3) << 1.0 / awayWin
                 << endl;
    }
}

void
calculateStakes(const vector<Game>& games, const Model& model, 
                const string& fileName)
{
    std::ofstream stakeFile(fileName.c_str());
    if (!stakeFile){
        LOG_ERROR << "Could not open " << fileName;
        return;
    }
    vector<PredictionData> predData(games.size());
    for (size_t i = 0; i < games.size(); ++i){
        double lambdaHome, lambdaAway;
        calculateLambdas(games[i], model, lambdaHome, lambdaAway);

        double homeWin = 0.0;
        double draw = 0.0;
        double awayWin = 0.0;
        calcProbs(lambdaHome, lambdaAway, homeWin, draw, awayWin);

        predData[i].mOdds.mHome = games[i].mHomeOdds;
        predData[i].mOdds.mDraw = games[i].mDrawOdds;
        predData[i].mOdds.mAway = games[i].mAwayOdds;
        predData[i].mProb.mHome = homeWin;
        predData[i].mProb.mDraw = draw;
        predData[i].mProb.mAway = awayWin;
        predData[i].calculateStats();
    }

    double stakeNormFactor = 0.0;
    double totalRisk = 0.0;
    for (size_t i = 0; i < games.size(); ++i){
        size_t bestBet = predData[i].bestBet();
        double stake =  predData[i].mMu[bestBet] / predData[i].mV[bestBet];
        predData[i].mStake = stake;
        stakeNormFactor += stake;
        totalRisk += predData[i].mV[bestBet] * pow(predData[i].mStake, 2);  
        
    } 
    totalRisk = sqrt(totalRisk);
    
    double budget = confParam<double>("finance.max_budget");
    double maxRisk = confParam<double>("finance.max_risk");
    stakeNormFactor = budget / stakeNormFactor;
    double riskNormFactor = maxRisk / totalRisk;
    double normFactor = min(riskNormFactor, stakeNormFactor);
    cout << stakeNormFactor << " " << riskNormFactor << endl;
    for (size_t i = 0; i < games.size(); ++i)
        predData[i].mStake *= normFactor;

    double totalSharpe = 0.0;
    double totalExpected = 0.0;
    double totalStake = 0.0;
    totalRisk = 0.0;
    for (size_t i = 0; i < games.size(); ++i){
        size_t bestBet = predData[i].bestBet();
        totalSharpe += pow(predData[i].mSharpe[bestBet], 2);  
        totalExpected += predData[i].mMu[bestBet] * predData[i].mStake;  
        totalRisk += predData[i].mV[bestBet] * pow(predData[i].mStake, 2);  
        totalStake += predData[i].mStake;  
    }
    totalSharpe = sqrt(totalSharpe);
    totalRisk = sqrt(totalRisk);

    stakeFile << "--------------------------------------" << endl
              << "Summary" << endl
              << "Max budget: " << confParam<double>("finance.max_budget")
              << endl
              << "Max risk: " << confParam<double>("finance.max_risk")
              << endl
              << "Portfolio size: "
              << setprecision(3) << totalStake << endl
              << "Portfolio expected return: " 
              << setprecision(3) << totalExpected << endl
              << "Portfolio risk: " 
              << setprecision(3) << totalRisk << endl
              << "Portfolio Sharpe ratio: " 
              << setprecision(3) << totalSharpe << endl
              << endl << endl;
    for (size_t i = 0; i < games.size(); ++i){
        TeamId homeT = games[i].mHomeTeam;
        TeamId awayT = games[i].mAwayTeam;
        size_t bestBet = predData[i].bestBet();
        stakeFile << homeT << " vs. " << awayT << ":  "
                  << setprecision(4) << predData[i].mStake
                  << "/" << setprecision(4) << totalStake
                  << " on " << indexToBet(bestBet) 
                  << endl;
    }

    
    stakeFile << endl << "--------------------------------------" << endl
              << "Detail" << endl;
    for (size_t i = 0; i < games.size(); ++i){
        TeamId homeT = games[i].mHomeTeam;
        TeamId awayT = games[i].mAwayTeam;
        size_t bestBet = predData[i].bestBet();
        double bookieMargin = 1.0 / (
            1.0 / games[i].mHomeOdds + 
            1.0 / games[i].mDrawOdds + 
            1.0 / games[i].mAwayOdds
            );
        stakeFile << homeT << " vs. " << awayT << endl;
        stakeFile << "                    " 
                  << setw(10) << "home" 
                  << setw(10) << "draw"  
                  << setw(10) << "away" << endl;  
        stakeFile << "bookie odds:        " 
                  << setw(10) << games[i].mHomeOdds
                  << setw(10) << games[i].mDrawOdds
                  << setw(10) << games[i].mAwayOdds 
                  << " Margin: " << setprecision(2) 
                  << bookieMargin * 100 << "%" << endl;
        stakeFile << "normalized:         " 
                  << setw(10) << games[i].mHomeOdds / bookieMargin
                  << setw(10) << games[i].mDrawOdds / bookieMargin
                  << setw(10) << games[i].mAwayOdds / bookieMargin << endl;
        stakeFile << "calculated odds:    " 
                  << setw(10) << setprecision(3) 
                  << 1.0 / predData[i].mProb.mHome 
                  << setw(10) << setprecision(3) 
                  << 1.0 / predData[i].mProb.mDraw  
                  << setw(10) << setprecision(3) 
                  << 1.0 / predData[i].mProb.mAway
                  << endl;
        stakeFile << "sharpe ratios:      " 
                  << setw(10) << predData[i].mSharpe.mHome
                  << setw(10) << predData[i].mSharpe.mDraw
                  << setw(10) << predData[i].mSharpe.mAway << endl;
        stakeFile << "put "
                  << setprecision(4) << predData[i].mStake
                  << "/" << setprecision(4) << totalStake
                  << " on " << indexToBet(bestBet) << endl;
        double stake = predData[i].mStake;
        stakeFile << "Expected return: " << setprecision(3) 
                  << predData[i].mMu[bestBet] * stake << endl;
        stakeFile << "Risk: " << setprecision(3) 
                  << sqrt(predData[i].mV[bestBet]) * stake << endl;
        stakeFile << endl << endl;
    }
    
}

double 
cumPoissonProb(size_t m, double lambda)
{
    double ret = 0.0;
    for (size_t i = 0; i < m; ++i)
        ret += poissonProb(i, lambda);
    ret += poissonProb(m, lambda) / 2.0;
    return ret;
}

void
calcKS(const vector<Game>& games, const Model& model, const string& fileName)
{
    std::ofstream histFile(fileName.c_str());
    if (!histFile){
        LOG_ERROR << "Could not open " << fileName;
        return;
    }
    
    vector<double> probs(games.size() * 2);
    for (size_t i = 0; i < games.size(); ++i){
        TeamId homeT = games[i].mHomeTeam;
        double homeA = model.a.find(homeT)->second;
        double homeD = model.d.find(homeT)->second;
        size_t homeG = games[i].mHomeGoals;

        TeamId awayT = games[i].mAwayTeam;
        double awayA = model.a.find(awayT)->second;
        double awayD = model.d.find(awayT)->second;
        size_t awayG = games[i].mAwayGoals;
        probs[i * 2] = cumPoissonProb(homeG, model.h * homeA * awayD); 
        probs[i * 2 + 1] = cumPoissonProb(awayG, awayA * homeD); 
    }
    sort(probs.begin(), probs.end());
    for (size_t i = 0; i < probs.size(); ++i)
        histFile << i << " " << probs[i] << endl;
    histFile << endl;
    for (size_t i = 0; i < probs.size(); ++i)
        histFile << i << " " << (i + 1.0 ) / probs.size() << endl;
}

int main(int argc, char* argv[])
{
    cout << "-------------" << endl;
    cout << "Running Ducks" << endl;

    if (argc != 2){
        cout << "usage: " << argv[0] 
             << " run" << endl;
        exit(-1);
    }

    path binPath(argv[0]);
    string command(argv[1]);

    path basePath = binPath.remove_leaf() / "../";
    path confFile = basePath / "conf/ducks.conf";
    cout << "Loading configuration file " << confFile << endl;
    Config config(confFile.string().c_str(), argc, argv);

    path logFile = 
        basePath / path(confParam<string>("system.log_file_name")
                        + confParam<string>("argv1") + string(".txt"));
    Logger* logger = new Logger(logFile.string().c_str());
    LOG_INFO << "Ducks " << confParam<string>("version");

    vector<Game> allGames;
    vector<Game> trainingGames;
    vector<Game> futureGames;
    loadStats(allGames, trainingGames, futureGames);
    vector<Game> betGames;
    loadBets(betGames);

    Model model;
    calculateModel(trainingGames, model);
    testModel(trainingGames, model);

    calcKS(trainingGames, model, confParam<string>("files.KS.training"));

    vector<Game> tiedGames;
    vector<Game> homeGames;
    vector<Game> awayGames;
    for (size_t i = 0; i < trainingGames.size(); ++i){
        const Game& game = trainingGames[i];
        if (game.mHomeGoals == game.mAwayGoals)
            tiedGames.push_back(game);
        else if (game.mHomeGoals > game.mAwayGoals)
            homeGames.push_back(game);
        else if (game.mHomeGoals < game.mAwayGoals)
            awayGames.push_back(game);
    }
    calcKS(tiedGames, model, confParam<string>("files.KS.training.tied"));
    calcKS(homeGames, model, confParam<string>("files.KS.training.home"));
    calcKS(awayGames, model, confParam<string>("files.KS.training.away"));

    predictFutureScores(futureGames, model,
                        confParam<string>("files.predictions"));

    calculateStakes(betGames, model,
                    confParam<string>("files.stakes"));

    calcKS(futureGames, model, confParam<string>("files.KS.future"));

    LOG_INFO << "Last log message: deleting Logger";
    delete logger;

    return 0;
} 
