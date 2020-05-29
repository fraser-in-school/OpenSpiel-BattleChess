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

# Lint as: python3
"""Tests for the game-specific functions for bridge."""

from absl.testing import absltest
import pyspiel


class GamesBridgeTest(absltest.TestCase):

  def test_contract_names(self):
    game = pyspiel.load_game('bridge')
    self.assertEqual(game.contract_string(0), 'Passed Out')
    self.assertEqual(game.contract_string(38), '1SX N')

  def test_possible_contracts(self):
    game = pyspiel.load_game('bridge')
    state = game.new_initial_state()
    for a in range(52):
      state.apply_action(a)
    state.apply_action(59)  # 1NT - now South cannot declare notrump
    state.apply_action(67)  # 3H - now West cannot declare hearts
    state.apply_action(86)  # 7D
    state.apply_action(53)  # Dbl
    possible_contracts = [
        game.contract_string(i)
        for i, v in enumerate(state.possible_contracts())
        if v
    ]
    self.assertCountEqual(possible_contracts, [
        '7DX S', '7DXX S', '7H N', '7HX N', '7HXX N', '7H E', '7HX E', '7HXX E',
        '7H S', '7HX S', '7HXX S', '7S N', '7SX N', '7SXX N', '7S E', '7SX E',
        '7SXX E', '7S S', '7SX S', '7SXX S', '7S W', '7SX W', '7SXX W', '7N N',
        '7NX N', '7NXX N', '7N E', '7NX E', '7NXX E', '7N W', '7NX W', '7NXX W'
    ])

  def test_scoring(self):
    game = pyspiel.load_game('bridge')
    state = game.new_initial_state()
    #         S T3
    #         H QT42
    #         D A82
    #         C A632
    # S KJ5           S Q7
    # H A965          H KJ8
    # D Q43           D KJT5
    # C T87           C Q954
    #         S A98642
    #         H 73
    #         D 976
    #         C KJ
    for a in [
        49, 45, 31, 5, 10, 40, 27, 47, 35, 38, 17, 14, 0, 33, 21, 39, 34, 12,
        22, 41, 1, 13, 36, 9, 4, 46, 11, 32, 2, 37, 29, 30, 7, 8, 19, 24, 16,
        43, 51, 15, 48, 23, 6, 20, 42, 26, 44, 50, 25, 28, 3, 18
    ]:
      state.apply_action(a)
    score = {
        game.contract_string(i): s
        for i, s in enumerate(state.score_by_contract())
    }
    self.assertEqual(score['1H E'], -110)
    self.assertEqual(score['1H W'], -80)
    self.assertEqual(score['3N W'], 50)
    self.assertEqual(score['1DX N'], -300)
    self.assertEqual(score['1CXX W'], -430)


if __name__ == '__main__':
  absltest.main()
