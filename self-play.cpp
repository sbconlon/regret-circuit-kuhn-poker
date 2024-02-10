#include <algorithm>
#include <cstdlib> 
#include <iostream>
#include <queue>
#include <set>
#include <string>
#include <vector>

using namespace std;

// Players
//
const int N_PLAYERS = 2;
enum player_t {Player1, Player2};

// Actions
//
const int N_ACTIONS = 2; // this is per decision.
enum first_action_t {Check=0, Bet=1}; 
enum second_action_t {Call=0, Fold=1};
set<first_action_t> first_actions = {Check, Bet}; // Actions that are made first.
set<second_action_t> second_actions = {Call, Fold}; // Actions made in response to the first actions.

string action_to_str(first_action_t action) {
    return (action ? "Bet" : "Check");
}

string action_to_str(second_action_t action) {
    return (action ? "Call" : "Fold");
}

// Cards
//
const int N_CARDS = 3;
enum card_t {None, Jack, Queen, King};
const set<card_t> valid_cards = {Jack, Queen, King};

string card_to_str(card_t card) {
    switch(card) {
        case None:
            return "None";
        case Jack:
            return "Jack";
        case Queen:
            return "Queen";
        case King:
            return "King";
        default:
            return "Unknown";
    }
}

// Infosets
//
// Set of decision points for a player that are indistinguishable to her based on
// her observed history.
//
// The strategy for a player is determined at the infoset level, thus we must store the
// strategy and regrets here.
//
class Infoset {
    protected:
        string my_name;
        double my_regret[N_ACTIONS];
        double my_strategy[N_ACTIONS];
        double my_cumm_strategy[N_ACTIONS];
    
    public:
        // Constructor
        explicit Infoset(string name) : my_name(name) { 
            // Initialize regret and strategy values.
            for(int i=0; i < N_ACTIONS; i++) {
                my_regret[i] = 0.;
                my_strategy[i] = 1./N_ACTIONS;
                my_cumm_strategy[i] = 0.;
            }
        }

        // Getters
        string name() {return my_name;}
        double regret(int i) {return my_regret[i];}
        double strategy(int i) {return my_strategy[i];}
        double cumm_strategy(int i) {return my_cumm_strategy[i];}
        double* avg_cumm_strategy();

        // Setters
        void add_regret(int i, double value) {my_regret[i] += value;}
        void add_cumm_strategy(int i, double value) {my_cumm_strategy[i] += value;}
        void update_strategy(int i, double value) {my_strategy[i] = value;}
};

double* Infoset::avg_cumm_strategy() {
    double* avg_strategy = (double*) malloc(N_ACTIONS*sizeof(double));
    // Accumulate the sum.
    double sum = 0.;
    int action;
    for (action = 0; action<N_ACTIONS; action++) {
        sum += my_cumm_strategy[action];
    }
    // Normalize.
    for (action = 0; action<N_ACTIONS; action++) {
        avg_strategy[action] = sum == 0 ? 1./N_ACTIONS : my_cumm_strategy[action]/sum;
    }
    return avg_strategy;
}

Infoset* infosets[N_PLAYERS][2][N_CARDS]; // Player, Facing Bet, Card

// Point types
//
// Base type is abstract.
//
enum point_t {base_t, decision_t, observation_t, terminal_t};

// Abstract base class for the components in the regret circuit
//
class Point {
    public:
        // Member variables
        string my_name;
        point_t my_type;
        vector<Point*> my_children;

        // Constructor
        Point() = default;
        explicit Point(string name) : my_name(name), my_type(base_t) {}

        // Member functions
        //
        virtual void next_strategy() = 0;
        virtual double observe_utility(const double reach_probs[N_PLAYERS]) = 0;
        void add_child(Point* child) {
            my_children.push_back(child);
        }
        virtual void print() = 0;
};


// Observation point
//
class Observation : public Point {
    public:
        // Class constructor
        explicit Observation(string name) { 
            my_name = name;
            my_type = observation_t; 
        }

        // Member functions
        //
        // Note: because this is the root node, we will not aggregrate the strategy.
        //  If we were to, we would take the convex hull of the child strategies.
        //
        virtual void next_strategy() {}
        virtual double observe_utility(const double reach_probs[N_PLAYERS]);
        virtual void print() { /*cout << my_name << endl;*/ }

};

// Observation point
//
// Each child of this point represents a different outcome of the dealt cards.
// 2 players * 3 cards = 6 outcomes of the observation.
//
// nullptr is returned as its assumed the observation node is at the top of the game tree.
//
double Observation::observe_utility(const double reach_probs[N_PLAYERS]) {
    // Each child node represents a different outcome of the observation.
    // Observe the utility of each child.
    //
    my_children[rand() % my_children.size()]->observe_utility(reach_probs);
    return 0.; // Root node - no need to bubble up a utility.
}

// Decision point
//
template <typename T>
class Decision : public Point {
    public:
        // Member variables
        player_t my_player; // Player that is making this decision.
        set<T> my_actions;  // Set of valid actions at this decision point.
        // Game state
        // Note: The opponent's card is hidden from the player owning this decision.
        card_t my_p1_card;     // Player 1 card
        card_t my_p2_card;     // Player 2 card
        bool my_facing_bet;    // Whether or not the player is facing a bet from the other player.

        // Constructor

        explicit Decision(string name, player_t p, card_t p1_card, card_t p2_card, bool facing_bet, set<T> actions) : 
            my_player(p),
            my_p1_card(p1_card),
            my_p2_card(p2_card),
            my_facing_bet(facing_bet) {
            my_name = name;
            my_actions = actions;
            my_type = decision_t;
        }

        // Member functions
        virtual void next_strategy();
        virtual double observe_utility(const double reach_probs[N_PLAYERS]);
        void print();
};

// Update the strategy for the player at this decision point
// using the current regret values.
//
// Note: there will be repetitive assignments as there are two decision points
//       for each infoset. This is just wasted computation and does not affect
//       the final answer.
//       (Is this still true?)
//
template <typename T>
void Decision<T>::next_strategy() {
    // Temporary array to hold intermediary calculations
    double theta[N_ACTIONS];
    // Get the card for the player owning this decision.
    card_t my_card = (my_player ? my_p2_card : my_p1_card);
    // Get the infoset that this decision point belongs to.
    Infoset* my_infoset = infosets[my_player][my_facing_bet][my_card];
    // Zero out negative regrets
    double sum = 0.;
    for (auto action : my_actions) {
        theta[action] = max(my_infoset->regret(action), 0.);
        sum = sum + theta[action];
    }
    // If all regrets are non-positive, then select a uniform, mixed strategy.
    if (sum == 0) {
        for (auto action : my_actions) {
            theta[action] = 1.;
            sum += theta[action];
        }
    }
    // Set the next strategy to the normalized theta.
    for (auto action : my_actions) {
        my_infoset->update_strategy(action, theta[action]/sum);
    }
}

// Observe utility for the decision node.
//
// This function accumulates the regret at this decision point.
//
// Note:
// The decision node for one player is an observation node for the other.
// The utility for the opposing player is summed and passed to the parent node.
//
template<typename T>
double Decision<T>::observe_utility(const double reach_probs[N_PLAYERS]) {
    // Get the card for the player owning this decision.
    card_t my_card = (my_player ? my_p2_card : my_p1_card);
    // Get the infoset that this decision point belongs to.
    Infoset* my_infoset = infosets[my_player][my_facing_bet][my_card];
    // Prob. of reaching the child node, assuming the given player is playing towards it.
    double next_reach_probs[N_PLAYERS] = {1., 1.}; 
    // Stores the utility observed for each action.
    double utilities[N_ACTIONS];  
    // Accumulates the ev for this infoset.
    double ev = 0.;
    // Each child is associated with an action, numbered [0, N_ACTIONS].
    int action = 0;
    for (auto child : my_children) {
        // Compute reach probabilities for this child.
        next_reach_probs[my_player] = reach_probs[my_player] * my_infoset->strategy(action);
        next_reach_probs[!my_player] = reach_probs[!my_player];
        // Observe the utility for this child.
        // Flip sign of utility depending on which player owns this decision node.
        utilities[action] = (my_player ? -1. : 1.) * child->observe_utility(next_reach_probs);
        // Add the ev contribution for this action
        ev += my_infoset->strategy(action) * utilities[action];
        action++;
    }
    // Update regrets
    for (auto action : my_actions) {
        /*
        if (my_name == "j8") {
            cout << "---" << endl;
            //cout << reach_probs[!my_player] << endl;
            cout << action_to_str(action) << ": " << utilities[action] << endl;
            //cout << ev << endl;
        }
        */
        my_infoset->add_regret(action,  reach_probs[!my_player] * (utilities[action] - ev));
    }
    // Update the strategy at this infoset.
    next_strategy();
    // Update cummulative strategy
    for (auto action : my_actions) {
        my_infoset->add_cumm_strategy(action, reach_probs[my_player] * my_infoset->strategy(action));
    }
    // Return the observed EVs
    // Flip sign back to always return EVs wrt Player1.
    return (my_player ? -1. : 1.) * ev;
}

template<typename T>
void Decision<T>::print() {
    // Print node name
    cout << my_name << ": ";
    // Print player
    cout << (my_player ? "Player2 " : "Player1 ");
    // Print the cards
    cout << "(" << card_to_str(my_p1_card) << ", " << card_to_str(my_p2_card) << ") ";
    // Print the infoset.
    Infoset* my_infoset = infosets[my_player][my_facing_bet][(my_player ? my_p2_card : my_p1_card)];
    cout << "Infoset = " << my_infoset->name() << " ";
    // Print regret
    cout << "Regret = {";
    for (int action=0; action<N_ACTIONS; action++) {
        cout << action_to_str((T)action) << ": " << my_infoset->regret(action);
        cout << (action==N_ACTIONS-1 ? "} " : ", ");
    }
    // Print strategy
    double* avg_strategy = my_infoset->avg_cumm_strategy();
    cout << "Average Strategy = {";
    for (int action=0; action<N_ACTIONS; action++) {
        cout << action_to_str((T)action) << ": " << avg_strategy[action];
        cout << (action==N_ACTIONS-1 ? "}" : ", ");
    }
    free(avg_strategy);
    cout << endl;
}

// Terminal point
//
// Note: 'my_payout' is defined for Player 1.
//       Because this is a zero-sum game, Player 1 payout = -1 * Player 2 payout
//
class Terminal : public Point {
    public:
        double my_payout;
        explicit Terminal(string name, double payout) : my_payout(payout) { 
            my_name = name;
            my_type = terminal_t;
        }
        virtual double observe_utility(const double reach_probs[N_PLAYERS]) { return my_payout; }
        virtual void next_strategy() {};
        void add_child(Point* child) {};
        virtual void print() { /*cout << my_name << endl;*/ }
};

// Game tree initialization functions
//
void init_infosets() {
    // List of names for the infosets:
    string names[12] = {"A1", "A2", "A3", "A4", "A5", "A6",
                        "B1", "B2", "B3", "B4", "B5", "B6"};
    int idx = 0;
    // Initialize the infosets.
    // Each infoset is uniquely determined by the tuple: (Player, Card, Facing Bet)
    for (player_t player : {Player1, Player2}) {
        for (bool facing : {false, true}) {
            for (card_t card : valid_cards) {
                infosets[player][facing][card] = new Infoset(names[idx]);
                idx++;
            }
        }
    }
}

Observation* init_tree() {
    init_infosets();
    //
    // Tree Depth 0 - Root
    Observation* c1 = new Observation("Chance");
    //
    // Tree Depth 1 - Player 1 first actions
    Decision<first_action_t>* j1 = new Decision("j1", Player1, Jack,  Queen, false, first_actions);
    Decision<first_action_t>* j2 = new Decision("j2", Player1, Jack,  King,  false, first_actions);
    Decision<first_action_t>* j3 = new Decision("j3", Player1, Queen, Jack,  false, first_actions);
    Decision<first_action_t>* j4 = new Decision("j4", Player1, Queen, King,  false, first_actions);
    Decision<first_action_t>* j5 = new Decision("j5", Player1, King,  Jack,  false, first_actions);
    Decision<first_action_t>* j6 = new Decision("j6", Player1, King,  Queen, false, first_actions);
    c1->add_child(j1);
    c1->add_child(j2);
    c1->add_child(j3);
    c1->add_child(j4);
    c1->add_child(j5);
    c1->add_child(j6);
    //
    // Tree Depth 2 - Player 2 response actions
    Decision<first_action_t>*  j7  = new Decision("j7",  Player2, Jack,  Queen, false, first_actions);
    Decision<second_action_t>* j8  = new Decision("j8",  Player2, Jack,  Queen, true,  second_actions);
    Decision<first_action_t>*  j9  = new Decision("j9",  Player2, Jack,  King,  false, first_actions);
    Decision<second_action_t>* j10 = new Decision("j10", Player2, Jack,  King,  true,  second_actions);
    Decision<first_action_t>*  j11 = new Decision("j11", Player2, Queen, Jack,  false, first_actions);
    Decision<second_action_t>* j12 = new Decision("j12", Player2, Queen, Jack,  true,  second_actions);
    Decision<first_action_t>*  j13 = new Decision("j13", Player2, Queen, King,  false, first_actions);
    Decision<second_action_t>* j14 = new Decision("j14", Player2, Queen, King,  true,  second_actions);
    Decision<first_action_t>*  j15 = new Decision("j15", Player2, King,  Jack,  false, first_actions);
    Decision<second_action_t>* j16 = new Decision("j16", Player2, King,  Jack,  true,  second_actions);
    Decision<first_action_t>*  j17 = new Decision("j17", Player2, King,  Queen, false, first_actions);
    Decision<second_action_t>* j18 = new Decision("j18", Player2, King,  Queen, true,  second_actions);
    j1->add_child(j7);
    j1->add_child(j8);
    j2->add_child(j9);
    j2->add_child(j10);
    j3->add_child(j11);
    j3->add_child(j12);
    j4->add_child(j13);
    j4->add_child(j14);
    j5->add_child(j15);
    j5->add_child(j16);
    j6->add_child(j17);
    j6->add_child(j18);
    //
    // Tree Depth 3 - Check-Check Terminal Nodes + Player 1 response actions
    Terminal* plus_two  = new Terminal("+2", 2.);
    Terminal* plus_one  = new Terminal("+1", 1.);
    Terminal* minus_one = new Terminal("-1", -1.);
    Terminal* minus_two = new Terminal("-2", -2.);
    Decision<second_action_t>* j19 = new Decision("j19", Player1, Jack,  Queen, true, second_actions);
    Decision<second_action_t>* j20 = new Decision("j20", Player1, Jack,  King,  true, second_actions);
    Decision<second_action_t>* j21 = new Decision("j21", Player1, Queen, Jack,  true, second_actions);
    Decision<second_action_t>* j22 = new Decision("j22", Player1, Queen, King,  true, second_actions);
    Decision<second_action_t>* j23 = new Decision("j23", Player1, King,  Jack,  true, second_actions);
    Decision<second_action_t>* j24 = new Decision("j24", Player1, King,  Queen, true, second_actions);
    j7->add_child(minus_one);
    j7->add_child(j19);
    j8->add_child(plus_one);
    j8->add_child(minus_two);
    j9->add_child(minus_one);
    j9->add_child(j20);
    j10->add_child(plus_one);
    j10->add_child(minus_two);
    j11->add_child(plus_one);
    j11->add_child(j21);
    j12->add_child(plus_one);
    j12->add_child(plus_two);
    j13->add_child(minus_one);
    j13->add_child(j22);
    j14->add_child(plus_one);
    j14->add_child(minus_two);
    j15->add_child(plus_one);
    j15->add_child(j23);
    j16->add_child(plus_one);
    j16->add_child(plus_two);
    j17->add_child(plus_one);
    j17->add_child(j24);
    j18->add_child(plus_one);
    j18->add_child(plus_two);
    //
    // Tree Depth 4 - Check-Bet-Fold Check-Bet-Call Terminal Nodes
    j19->add_child(minus_one);
    j19->add_child(minus_two);
    j20->add_child(minus_one);
    j20->add_child(minus_two);
    j21->add_child(minus_one);
    j21->add_child(plus_two);
    j22->add_child(minus_one);
    j22->add_child(minus_two);
    j23->add_child(minus_one);
    j23->add_child(plus_two);
    j24->add_child(minus_one);
    j24->add_child(plus_two);

    return c1;
}

void print_tree(Point* root) {
    queue<Point*> q;
    Point* pt;
    q.push(root);
    while (!q.empty()) {
        pt = q.front();
        q.pop();
        pt->print();
        for (Point* child : pt->my_children) {
            q.push(child);
        }
    }
}

// Number of iterations
int N_ITERS = 1000000;

int main(int argc, char* argv[]) {
    Observation* root = init_tree();
    const double ones[N_PLAYERS] = {1., 1.};
    for (int i=0; i<N_ITERS; i++) {
        root->observe_utility(ones);
    }
    print_tree(root);
    return 0;
}