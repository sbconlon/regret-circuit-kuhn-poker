# Regret Circuits Applied to Kuhn Poker
 Regret circuit paradigm applied to Kuhn poker
 <br/><br/>
 Inspired by [Regret Circuits: Composability of Regret Minimizers](https://arxiv.org/pdf/1811.02540.pdf).
<br/><br/>
***
**Under construction!**
***
### 1. Kuhn Poker
See the Wikipedia [page](https://en.wikipedia.org/wiki/Kuhn_poker) for a comprehensive description of the game. <br/><br/>

### 2. Code Design
### 2.1. Players
Kuhn poker is a two-player game. This is represented by the `player_t` enum that assigns a value of 0 and 1 to `Player1` and `Player2`, respectively. <br/><br/>
![Players code snippet](./images/players.JPG)

### 2.2. Actions
At each decision point, the players must chose an action from a set of two available actions. The set of actions depends on whether or not the player is facing a bet or not. When not facing a bet, the player can `Check` or `Bet`, defined by the `first_actions` set. When facing a bet, the player can `Call` or `Fold`, defined by the `second_actions` set. <br/><br/>
![Actions code snippet](./images/actions.JPG)

### 2.3. Cards
There are three cards in Kuhn poker (`Jack`, `Queen`, `King`), defined by the enum `card_t`. <br/><br/>
![Cards code snippet](./images/cards.JPG)

### 2.4. Infosets
An infoset is a set of decision points for a player that are indistinguishable to her based on her observed history. Infosets are the result of information that is hidden from the player by the environment or the player's inability to observe their opponent's actions. <br/><br/>

For Kuhn poker, all actions are observable. Therefore, the infosets are the result of the hidden information, the card that the player's opponent is holding. There are three total cards, the player holds one, so there are two possibilities for the opponent's card, meaning there are two decision nodes in each infoset. <br/><br/>

Shown below is the extensive-form game representation from Kuhn poker (taken from [Wikipedia](https://en.wikipedia.org/wiki/Kuhn_poker)) with the decision points and infosets labeled: <br/><br/>
![Extensive-form game tree](./images/Kuhn_poker_tree_v2.png)

By definition, each decision point in an infoset must have the same set of available actions, and the accumulated regret and strategy over those actions. If they were different, then the decision points would be distinguishable and not part of the same infoset. <br/><br/>

To reflect this many-to-one relationship of decision points to infoset in the code, the infoset is given its own class apart from that of the decision point class. <br/><br/>
![Infoset code snippet](./images/infoset.JPG)

Each infoset is uniquely determined by three values: the player who is making the decision, whether or not that player is facing a bet, and the card that the player holds. This is shown in the global array of infoset pointers that is defined in the code. Note, this array is made global to allow each decision point to access it. Training is done sequentially, so there is no need to worry about race conditions. <br/><br/>
![Array of infosets](./images/infoset-array.JPG)


