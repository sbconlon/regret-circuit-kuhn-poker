# Regret Circuits Applied to Kuhn Poker
 Regret circuit paradigm applied to Kuhn poker
 <br/><br/>
 Inspired by [Regret Circuits: Composability of Regret Minimizers](https://arxiv.org/pdf/1811.02540.pdf).
<br/><br/>

***
<img src="https://github.com/sbconlon/regret-circuit-kuhn-poker/blob/main/images/construction.gif" width="25%">

**Under construction!**
***
### 1. Kuhn Poker
See the Wikipedia [page](https://en.wikipedia.org/wiki/Kuhn_poker) for a comprehensive description of the game. <br/><br/>

### 2. Code Design
### 2.1. Players
Kuhn poker is a two-player game. This is represented by the `player_t` enum that assigns a value of 0 and 1 to `Player1` and `Player2`, respectively. <br/><br/>
![Players code snippet](./images/players.JPG)

### 2.2. Actions
At each decision point, the players must chose an action from a set of two available actions. The set of actions depends on whether or not the player is facing a bet. When not facing a bet, the player can `Check` or `Bet`, defined by the `first_actions` set. When facing a bet, the player can `Call` or `Fold`, defined by the `second_actions` set. <br/><br/>
![Actions code snippet](./images/actions.JPG)

### 2.3. Cards
There are three cards in Kuhn poker (`Jack`, `Queen`, `King`), defined by the enum `card_t`. <br/><br/>
![Cards code snippet](./images/cards.JPG)

### 2.4. Infosets
An infoset is a set of decision points for a player that are indistinguishable to her based on her observed history. Infosets are the result of information that is hidden from the player by the environment or the player's inability to observe their opponent's actions. <br/><br/>

For Kuhn poker, all actions are observable. Therefore, the infosets are the result of the hidden information, the card that the player's opponent is holding. There are three total cards, the player holds one, so there are two possibilities for the opponent's card, meaning there are two decision nodes in each infoset. <br/><br/>

Shown below is the extensive-form game representation for Kuhn poker (taken from [Wikipedia](https://en.wikipedia.org/wiki/Kuhn_poker)) with the decision points and infosets labeled: <br/><br/>
![Extensive-form game tree](./images/Kuhn_poker_tree_v2.png)

By definition, each decision point in an infoset must have the same set of available actions, and the accumulated regret and strategy over those actions. If they were different, then the decision points would be distinguishable and not part of the same infoset. <br/><br/>

To reflect this many-to-one relationship of many decision points to a single infoset in the code, the infoset is given its own class apart from that of the decision point class. <br/><br/>
![Infoset code snippet](./images/infoset.JPG)

Each infoset is uniquely determined by three values: the player who is making the decision, whether or not that player is facing a bet, and the card that the player holds. This is shown in the global array of infoset pointers that is defined in the code. Note, this array is made global to allow each decision point to access it. Training is done sequentially, so there is no need to worry about race conditions. <br/><br/>
![Array of infosets](./images/infoset-array.JPG)

The average accumulated strategy for the infoset converages to an equilibrium.
![Average accumulated strategy code snippet](./images/avg-strategy.JPG)

### 2.5. Points
Each point, or node, in an extensive-form game represents a game state. The edges between points represents the transitions from one state to the next. <br/><br/>

There are three types of points in the game tree:<br/><br/>
![Point types code snippet](./images/point-types.JPG)

The abstract base point class:<br/><br/>
![Base point class code snippet](./images/point.JPG)

The functions common to all point classes are `observe_utility` and `next_strategy`. <br/><br/>

`observe_utility` recursively traverses the game tree. On the downward pass, the game state and reach probabililities are propagated to the child points. On the upward pass, the point's utiltity is propagated upward to the parent points.<br/><br/>

`next_strategy` is called as part of the upward traversal of the game tree, once the child utilities have been used to update the accumulated regret for the infoset. This strategy is updated according to the regret matching algorithm, discussed in more detail below. <br/><br/>

### 2.6. Observation Points
Observation points represent the stochasticsity of the environment. Each edge emanating from a chance point represents a different outcome of the observation. Players do not know the dynamics, the probability distribution over outcomes, of the environment a priori. <br/><br/>

For Kuhn Poker, only one chance event occurs - the dealing of the cards to each player at the beginning of the game. <br/><br/>

The observation class:<br/><br/>
![Observation point class code snippet](./images/observation-point.JPG)

The `observe_utility` function randomly samples a child node. Here, each child node represents a different outcome of the dealt cards to each player, 6 possiblities in total. We then recurse down the game tree by calling `observe_utiltity` on the child node.<br/><br/>
![Observation point observe_utility code snippet](./images/observation-observe-utility.JPG)

The `next_strategy` function for the observation class has been left empty for simplicity since the observation node is always the root of the game tree in Kuhn Poker. If one wanted to accumulate a strategy for the observation node, then the convex hull of the child strategies would be taken. <br/><br/>

### 2.7. Decision Points
Decision points represent the player's interaction with the environment. Each decision point belongs to a single player. Each edge emanating from a decision point represents a different action available to the player. <br/><br/>

For Kuhn Poker, each decision point is uniquely identified by the players' cards, the player who owns the decision, and whether that player is facing a bet. <br/><br/>

The decision class:<br/><br/>
![Decision point class code snippet](./images/decision-point.JPG)




