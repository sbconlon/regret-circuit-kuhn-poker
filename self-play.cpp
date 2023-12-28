#include <algorithm>
#include <cstdlib> 
#include <iostream>
#include <set>
#include <vector>

using namespace std;

// Players
//
enum player_t {Player1 = 0, Player2 = 1};

// Actions
//
const int N_ACTIONS = 2; // this is per decision.
enum first_action_t {Check, Bet}; 
enum second_action_t {Call, Fold};
set<first_action_t> first_actions = {Check, Bet}; // Actions that are made first.
set<second_action_t> second_actions = {Call, Fold}; // Actions made in response to the first actions.

// Cards
//
const int N_CARDS = 3;
enum card_t {None, Jack, Queen, King};
const set<card_t> valid_cards = {Jack, Queen, King};

// Point types
//
// Base type is abstract.
//
enum point_t {base_t, decision_t, observation_t, terminal_t};

// Base class for the components in the regret circuit
//
class Point {
    protected:
        double my_regret[N_ACTIONS];
        double my_strategy[N_ACTIONS];

    public:
        // Member variables
        point_t type;
        vector<Point*> children;

        // Constructor
        explicit Point() : type(base_t) {}

        // Member functions
        //
        virtual void next_strategy() = 0;
        virtual double* observe_utility(card_t p1_card, card_t p2_card) = 0;
        void add_child(Point* child) {
            children.push_back(child);
        }
};


// Observation point
//
class Observation : public Point {
    public:
        // Member variables
        player_t my_player; // Player that is making this observation.

        // Class constructor
        explicit Observation(player_t p) : my_player(p) { type = observation_t; }

        // Member functions
        //
        virtual void next_strategy() {};
        virtual double* observe_utility(card_t p1_card, card_t p2_card);
};

// Observation points observe their player's card.
//
// This card value is then passed to the decision points lower in the game tree.
//
// nullptr is returned as its assumed the observation nodes are at the top of the game tree.
//
double* Observation::observe_utility(card_t p1_card, card_t p2_card) {
    // Get opponents card.
    card_t opp_card = my_player == Player1 ? p2_card : p1_card;
    // Each child node represents a different outcome of the observation.
    // Prevent the players from having the same card, or no card.
    int i = 0;
    double* res;
    for (auto card : valid_cards) {
        if (card == None || card == opp_card)
            continue;
        if (my_player == Player1)
            res = children[i]->observe_utility(card, p2_card);
        else
            res = children[i]->observe_utility(p1_card, card);
        free(res);
    }
    return nullptr;
}

// Decision point
//
template <typename T>
class Decision : public Point {
    public:
        // Member variables
        player_t my_player; // Player that is making this decision.
        set<T> my_actions;  // Set of valid actions at this decision point.

        // Constructor
        explicit Decision(player_t p, set<T> actions) : my_player(p), my_actions(actions) { 
            type = decision_t;
            // Initialize regret and strategy values.
            for(int i=0; i < N_ACTIONS; i++) {
                my_regret[i] = 0.;
                my_strategy[i] = 1./N_ACTIONS;
            }
        }

        // Member functions
        virtual void next_strategy();
        virtual double* observe_utility(card_t p1_card, card_t p2_card);
};

// Update the strategy for the player at this decision point
// using the current regret values.
//
template <typename T>
void Decision<T>::next_strategy() {
    // Temporary array to hold intermediary calculations
    double theta[N_ACTIONS];
    // Zero out negative regrets
    int sum = 0;
    for (int i=0; i < N_ACTIONS; i++) {
        theta[i] = max(my_regret[i], 0);
        sum += theta[i];
    }
    // If all regrets non-positive, then select a random strategy.
    if (sum == 0) {
        for (int i=0; i < N_ACTIONS; i++) {
            theta[i] = (double) rand(); // random integer [0-32767]
            sum += theta[i];
        }
    }
    // Set the next strategy to the normalized theta.
    for (int i=0; i<N_ACTIONS; i++) {
        my_strategy[i] = (theta[i]/sum);
    }
}

// Observe utility for the decision node.
//
// Note:
// The decision node for one player is an observation node for the other.
// The utility for the opposing player is summed and passed to the parent node.
//
template<typename T>
double* Decision<T>::observe_utility(card_t p1_card, card_t p2_card) {
    // Get utilities from the child nodes.
    // Accumulate our player's utilities into a vector.
    // Accumulate the opposing player's utilities into a sum.
    double utilities[N_ACTIONS];
    double opp_utility = 0;
    double* child_utilities;
    int i = 0;
    for (auto pt : children) {
        child_utilities = pt->observe_utility(p1_card, p2_card);
        utilities[i] = child_utilities[my_player];
        opp_utility += child_utilities[!my_player];
        free(child_utilties);
    }
    // Update regrets
    double ev = 0;
    for (int i=0; i < N_ACTIONS; i++)
        ev += my_strategy[i] * utilities[i];
    for (int i=0; i < N_ACTIONS; i++)
        my_regret[i] += (utilities[i] - ev);
    // Update strategy
    next_strategy();
    // Return the observed utilities for the two players
    double* result = malloc(2 * sizeof(double));
    result[my_player] = ev;
    result[!my_player] = opp_utility;
    return result;
}

// Terminal point
//
class Terminal : public Point {
    public:
        double my_payout;
        explicit Terminal(double payout) : my_payout(payout) { type = terminal_t; }
        virtual double* observe_utility(card_t p1_card, card_t p2_card);
        virtual void next_strategy() {};
        void add_child(Point* child) {};
};

double* Terminal::observe_utility(card_t p1_card, card_t p2_card) {
    double* result =  (double*) malloc(2 * sizeof(double));
    int winner = p1_card > p2_card ? -1 : 1;
    result[Player1] = winner * my_payout;
    result[Player2] = winner * my_payout;
    return result;
}


void print_type(Point& pt) {
    switch (pt.type) {
        case base_t:
            cout << "Base Point" << endl;
            break;
        case observation_t:
            cout << "Observation Point" << endl;
            break;
        case decision_t:
            cout << "Decision Point" << endl;
            break;
        case terminal_t:
            cout << "Terminal Point" << endl;
            break;
        default:
            cout << "TYPE NOT RECOGNIZED" << endl;
    }
}

int main(int argc, char* argv[]) {
    // Initialize terminal nodes
    Terminal t1(1);
    Terminal t2(2);
    // Initialize Player 1 response nodes
    Decision<second_action_t> j10(Player1, second_actions);
    j10.add_child(&t1);
    j10.add_child(&t2);
    Decision<second_action_t> j11(Player1, second_actions);
    j11.add_child(&t1);
    j11.add_child(&t2);
    Decision<second_action_t> j12(Player1, second_actions);
    j12.add_child(&t1);
    j12.add_child(&t2);
    // Initialize Player 2 response nodes
    Decision<second_action_t> j4(Player2, second_actions);
    j4.add_child(&t1);
    j4.add_child(&j10);

    

    return 0;
}