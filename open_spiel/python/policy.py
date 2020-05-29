# Copyright 2019 DeepMind Technologies Ltd. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Lint as python3
"""Representation of a policy for a game.

This is a standard representation for passing policies into algorithms,
with currently the following implementations:

  TabularPolicy - an explicit policy per state, stored in an array
    of shape `(num_states, num_actions)`, convenient for tabular policy
    solution methods.
  UniformRandomPolicy - a uniform distribution over all legal actions for
    the specified player. This is computed as needed, so can be used for
    games where a tabular policy would be unfeasibly large.

The main way of using a policy is to call `action_probabilities(state,
player_id`), to obtain a dict of {action: probability}. `TabularPolicy`
objects expose a lower-level interface, which may be more efficient for
some use cases.
"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import numpy as np

from open_spiel.python.algorithms import get_all_states
import pyspiel


class Policy(object):
  """Base class for policies.

  A policy is something that returns a distribution over possible actions
  given a state of the world.

  Attributes:
    game: the game for which this policy applies
    player_ids: list of player ids for which this policy applies; each in the
      interval [0..game.num_players()-1].
  """

  def __init__(self, game, player_ids):
    """Initializes a policy.

    Args:
      game: the game for which this policy applies
      player_ids: list of player ids for which this policy applies; each should
        be in the range 0..game.num_players()-1.
    """
    self.game = game
    self.player_ids = player_ids

  def action_probabilities(self, state, player_id=None):
    """Returns a dictionary {action: prob} for all legal actions.

    IMPORTANT: We assume the following properties hold:
    - All probabilities are >=0 and sum to 1
    - TLDR: Policy implementations should list the (action, prob) for all legal
      actions, but algorithms should not rely on this (yet).
      Details: Before May 2020, only legal actions were present in the mapping,
      but it did not have to be exhaustive: missing actions were considered to
      be associated to a zero probability.
      For example, a deterministic state-poliy was previously {action: 1.0}.
      Given this change of convention is new and hard to enforce, algorithms
      should not rely on the fact that all legal actions should be present.

    Args:
      state: A `pyspiel.State` object.
      player_id: Optional, the player id for whom we want an action. Optional
        unless this is a simultaneous state at which multiple players can act.

    Returns:
      A `dict` of `{action: probability}` for the specified player in the
      supplied state.
    """
    raise NotImplementedError()

  def __call__(self, state, player_id=None):
    """Turns the policy into a callable.

    Args:
      state: The current state of the game.
      player_id: Optional, the player id for whom we want an action. Optional
        unless this is a simultaneous state at which multiple players can act.

    Returns:
      Dictionary of action: probability.
    """
    return self.action_probabilities(state, player_id)

  def to_tabular(self):
    """Returns a new `TabularPolicy` equivalent to this policy."""
    tabular_policy = TabularPolicy(self.game, self.player_ids)
    for index, state in enumerate(tabular_policy.states):
      tabular_policy.action_probability_array[index, :] = 0
      for action, probability in self.action_probabilities(state).items():
        tabular_policy.action_probability_array[index, action] = probability
    return tabular_policy


class TabularPolicy(Policy):
  """Policy implementation where the policy is in explicit tabular form.

  In addition to implementing the `Policy` interface, this class exposes
  details of the policy representation for easy manipulation.

  The states are guaranteed to be grouped by player, which can simplify
  code for users of this class, i.e. `action_probability_array` contains
  states for player 0 first, followed by states for player 1, etc.

  The policy uses `state.information_state` as the keys if available, otherwise
  `state.observation`.

  Usages:

  - Set `policy(info_state, action)`:
  ```
  tabular_policy = TabularPolicy(game)
  info_state_str = state.information_state_string(<optional player>)
  state_policy = tabular_policy.policy_for_key(info_state_str)
  state_policy[action] = <value>
  ```
  - Set `policy(info_state)`:
  ```
  tabular_policy = TabularPolicy(game)
  info_state_str = state.information_state_string(<optional player>)
  state_policy = tabular_policy.policy_for_key(info_state_str)
  state_policy[:] = <list or numpy.array>
  ```

  Attributes:
    action_probability_array: array of shape `(num_states, num_actions)`, where
      `action_probability_array[s, a]` is the probability of choosing action `a`
      when at state `s`.
    state_lookup: `dict` mapping state key string to index into the
      `tabular_policy` array. If information state strings overlap, e.g. for
      different players or if the information state string has imperfect recall,
      then those states will be mapped to the same policy.
    legal_actions_mask: array of shape `(num_states, num_actions)`, each row
      representing which of the possible actions in the game are valid in this
      particular state, containing 1 for valid actions, 0 for invalid actions.
    states_per_player: A `list` per player of the state key strings at which
      they have a decision to make.
    states: A `list` of the states as ordered in the `action_probability_array`.
    state_in: array of shape `(num_states, state_vector_size)` containing the
      normalised vector representation of each information state. Populated only
      for games which support information_state_tensor(), and is None otherwise.
    game_type: The game attributes as returned by `Game::GetType`; used to
      determine whether to use information state or observation as the key in
      the tabular policy.
  """

  def __init__(self, game, players=None):
    """Initializes a uniform random policy for all players in the game."""
    players = sorted(players or range(game.num_players()))
    super(TabularPolicy, self).__init__(game, players)
    self.game_type = game.get_type()

    # Get all states in the game at which players have to make decisions.
    states = get_all_states.get_all_states(
        game,
        depth_limit=-1,
        include_terminals=False,
        include_chance_states=False)

    # Assemble legal actions for every valid (state, player) pair, keyed by
    # information state string.
    self.state_lookup = {}
    self.states_per_player = [[] for _ in range(game.num_players())]
    self.states = []
    legal_actions_list = []
    state_in_list = []
    for player in players:
      # States are ordered by their history.
      for _, state in sorted(states.items(), key=lambda pair: pair[0]):
        if state.is_simultaneous_node() or player == state.current_player():
          legal_actions = state.legal_actions_mask(player)
          if any(legal_actions):
            key = self._state_key(state, player)
            if key not in self.state_lookup:
              state_index = len(legal_actions_list)
              self.state_lookup[key] = state_index
              legal_actions_list.append(legal_actions)
              self.states_per_player[player].append(key)
              self.states.append(state)
              if self.game_type.provides_information_state_tensor:
                state_in_list.append(state.information_state_tensor(player))
              elif self.game_type.provides_observation_tensor:
                state_in_list.append(state.observation_tensor(player))

    # Put legal action masks in a numpy array and create the uniform random
    # policy.
    self.state_in = None
    if state_in_list:
      self.state_in = np.array(state_in_list)
    self.legal_actions_mask = np.array(legal_actions_list)
    self.action_probability_array = (
        self.legal_actions_mask /
        np.sum(self.legal_actions_mask, axis=-1, keepdims=True))

  def _state_key(self, state, player):
    """Returns the key to use to look up this (state, player) pair."""
    if self.game_type.provides_information_state_string:
      if player is None:
        return state.information_state_string()
      else:
        return state.information_state_string(player)
    elif self.game_type.provides_observation_string:
      if player is None:
        return state.observation_string()
      else:
        return state.observation_string(player)
    else:
      return str(state)

  def action_probabilities(self, state, player_id=None):
    """Returns an {action: probability} dict, covering all legal actions."""
    probability = self.policy_for_key(self._state_key(state, player_id))
    legal_actions = (state.legal_actions() if player_id is None
                     else state.legal_actions(player_id))
    return {action: probability[action] for action in legal_actions}

  def state_index(self, state):
    """Returns the index in the TabularPolicy associated to `state`."""
    return self.state_lookup[self._state_key(state, state.current_player())]

  def policy_for_key(self, key):
    """Returns the policy as a vector given a state key string.

    Args:
      key: A key for the specified state.

    Returns:
      A vector of probabilities, one per action. This is a slice of the
      backing policy array, and so slice or index assignment will update the
      policy. For example:
      ```
      tabular_policy.policy_for_key(s)[:] = [0.1, 0.5, 0.4]
      ```
    """
    return self.action_probability_array[self.state_lookup[key]]

  def __copy__(self, copy_action_probability_array=True):
    """Returns a shallow copy of self.

    Most class attributes will be pointers to the copied object's attributes,
    and therefore altering them could lead to unexpected behavioural changes.
    Only action_probability_array is expected to be modified.

    Args:
      copy_action_probability_array: Whether to also include
        action_probability_array in the copy operation.

    Returns:
      Copy.
    """
    result = TabularPolicy.__new__(TabularPolicy)
    result.state_lookup = self.state_lookup
    result.game_type = self.game_type
    result.legal_actions_mask = self.legal_actions_mask
    result.state_in = self.state_in
    result.state_lookup = self.state_lookup
    result.states_per_player = self.states_per_player
    result.states = self.states
    result.game = self.game
    result.player_ids = self.player_ids
    if copy_action_probability_array:
      result.action_probability_array = np.copy(self.action_probability_array)
    return result

  def copy_with_noise(self,
                      alpha=0.0,
                      beta=0.0,
                      random_state=np.random.RandomState()):
    """Returns a copy of this policy perturbed with noise.

    Generates a new random distribution using a softmax on normal random
    variables with temperature beta, and mixes it with the old distribution
    using 1-alpha * old_distribution + alpha * random_distribution.
    Args:
      alpha: Parameter characterizing the mixture amount between new and old
        distributions. Between 0 and 1.
        alpha = 0: keep old table.
        alpha = 1: keep random table.
      beta: Temperature of the softmax. Makes for more extreme policies.
      random_state: A numpy `RandomState` object. If not provided, a shared
        random state will be used.

    Returns:
      Perturbed copy.
    """
    copied_instance = self.__copy__(False)
    probability_array = self.action_probability_array
    noise_mask = random_state.normal(size=probability_array.shape)
    noise_mask = np.exp(beta * noise_mask) * self.legal_actions_mask
    noise_mask = noise_mask / (np.sum(noise_mask, axis=1).reshape(-1, 1))
    copied_instance.action_probability_array = (
        1 - alpha) * probability_array + alpha * noise_mask
    return copied_instance


class UniformRandomPolicy(Policy):
  """Policy where the action distribution is uniform over all legal actions.

  This is computed as needed, so can be used for games where a tabular policy
  would be unfeasibly large, but incurs a legal action computation every time.
  """

  def __init__(self, game):
    """Initializes a uniform random policy for all players in the game."""
    all_players = list(range(game.num_players()))
    super(UniformRandomPolicy, self).__init__(game, all_players)

  def action_probabilities(self, state, player_id=None):
    """Returns a uniform random policy for a player in a state.

    Args:
      state: A `pyspiel.State` object.
      player_id: Optional, the player id for which we want an action. Optional
        unless this is a simultaneous state at which multiple players can act.

    Returns:
      A `dict` of `{action: probability}` for the specified player in the
      supplied state. This will contain all legal actions, each with the same
      probability, equal to 1 / num_legal_actions.
    """
    legal_actions = (state.legal_actions() if player_id is None
                     else state.legal_actions(player_id))
    probability = 1 / len(legal_actions)
    return {action: probability for action in legal_actions}


class FirstActionPolicy(Policy):
  """A policy that always takes the lowest-numbered legal action."""

  def __init__(self, game):
    all_players = list(range(game.num_players()))
    super(FirstActionPolicy, self).__init__(game, all_players)

  def action_probabilities(self, state, player_id=None):
    legal_actions = (state.legal_actions() if player_id is None
                     else state.legal_actions(player_id))
    min_action = min(legal_actions)
    return {action: 1.0 if action == min_action else 0.0
            for action in legal_actions}


def tabular_policy_from_callable(game, callable_policy, players=None):
  """Converts a legacy callable policy into a TabularPolicy.

  Recommendation - instead of using this to convert your policy for evaluation
  purposes, work directly with a `TabularPolicy` if possible.
  Second choice - work with a `Policy` class and call `to_tabular` as needed.

  Args:
    game: The game for which we want a TabularPolicy.
    callable_policy: A callable: state -> action probabilities dict or list.
    players: List of players this policy applies to. If `None`, applies to
      all players.
  Returns:
    A TabularPolicy that materializes the callable policy.
  """
  tabular_policy = TabularPolicy(game, players)
  for state_index, state in enumerate(tabular_policy.states):
    action_probabilities = dict(callable_policy(state))
    infostate_policy = [
        action_probabilities.get(action, 0.)
        for action in range(game.num_distinct_actions())
    ]
    tabular_policy.action_probability_array[state_index, :] = infostate_policy
  return tabular_policy


def python_policy_to_pyspiel_policy(python_tabular_policy):
  """Converts a TabularPolicy to a pyspiel.TabularPolicy."""
  infostates_to_probabilities = dict()
  for infostate, index in python_tabular_policy.state_lookup.items():
    probs = python_tabular_policy.action_probability_array[index]
    legals = python_tabular_policy.legal_actions_mask[index]

    action_probs = []
    for action, (prob, is_legal) in enumerate(zip(probs, legals)):
      if is_legal == 1:
        action_probs.append((action, prob))
    infostates_to_probabilities[infostate] = action_probs
  return pyspiel.TabularPolicy(infostates_to_probabilities)
