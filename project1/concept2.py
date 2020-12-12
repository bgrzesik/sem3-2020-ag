import math
# from ortools.linear_solver import pywraplp
from ortools.graph.pywrapgraph import SimpleMinCostFlow
import pydot
import imgcat


def tournament(player_count, budget, games_data):

    def cycle(x):
        solver = SimpleMinCostFlow()

        indices = [ * [f"p{player}l" for player in range(player_count)],
                    * [f"p{player}r" for player in range(player_count)],
                   "source", "target", "limit",]

        points = [0] * player_count
        for player_a, player_b, winner, bribe in games_data:
            points[winner] += 1


        for player_a, player_b, winner, bribe in games_data:
            loser = player_b if player_a == winner else player_a
            
            solver.AddArcWithCapacityAndUnitCost(
                    indices.index(f"p{winner}l"),
                    indices.index(f"p{loser}r"), 
                    1, bribe)

        for player in range(player_count):
            solver.AddArcWithCapacityAndUnitCost(
                indices.index(f"source"),
                indices.index(f"p{player}l"), points[player], 0)

            solver.AddArcWithCapacityAndUnitCost(
                indices.index(f"p{player}l"),
                indices.index(f"p{player}r"), points[player], 0)

            solver.AddArcWithCapacityAndUnitCost(
                indices.index(f"p{player}r"),
                indices.index("target" if player == 0 else "limit"), x, 0)

        games_count = ((player_count - 1) * player_count) // 2
        solver.AddArcWithCapacityAndUnitCost(
            indices.index("limit"),
            indices.index("target"), games_count - x, 0)

        solver.SetNodeSupply(indices.index("source"), games_count)
        solver.SetNodeSupply(indices.index("target"), -games_count)

        status = solver.SolveMaxFlowWithMinCost()

        if status != solver.OPTIMAL:
            return False

        if games_count != solver.MaximumFlow():
            return False

        if solver.OptimalCost() > budget:
            return False

        dot = pydot.Dot(rankdir="LR")
        dot = None

        if dot:
            for i, v in enumerate(indices):
                dot.add_node(pydot.Node(i, label=v))

        for i in range(solver.NumArcs()):
            tail = solver.Tail(i)
            head = solver.Head(i)

            capacity = solver.Capacity(i)
            cost = solver.UnitCost(i)
            flow = solver.Flow(i)

            if dot:
                dot.add_edge(pydot.Edge(tail, head, 
                                        label=f"{capacity}, {cost}, {flow}"))

        if dot:
            if not game_count > 50:
                imgcat.imgcat(dot.create_png())
            elif not game_count > 200:
                dot.write_png(f"{player_count}_big_boy.png")

        print("cost = ", solver.OptimalCost())


        return True

    for x in range(player_count // 2, player_count):
        if cycle(x):
            print(x)
            return True
        break

    return False


def parse_data(games_data):
    games_data = games_data.strip()
    return [[int(v) for v in line.strip().split(" ")]
            for line in games_data.splitlines()]


if __name__ == "__main__":
    with open("input.txt", "r") as f:
        game_count = int(f.readline())

        for game in range(game_count):
            budget = int(f.readline())
            player_count = int(f.readline())
            game_count = (player_count * (player_count - 1)) // 2
            data = "".join([f.readline() for _ in range(game_count)])

            if game_count == 0:
                print(True)
                continue

            print(tournament(player_count, budget, parse_data(data)))

            
