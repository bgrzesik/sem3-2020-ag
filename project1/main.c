#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

#if 0
#undef DDEEBBUUGG
#endif
//#define valloc malloc

#ifdef DDEEBBUUGG 
FILE *dot_file;
#define dotdebug(...) fprintf(dot_file, __VA_ARGS__)
#define dprintf(...) printf(__VA_ARGS__)
#else
#define dotdebug(...)
#define dprintf(...) 
#endif


#define min(a, b) ((a) < (b) ? (a) : (b))

typedef int16_t vertex_t;

struct edge {
    vertex_t tail; /* from */
    vertex_t head; /* to */

    int flow;
    int capacity;
    int cost;

    struct edge *next_tail; 
    struct edge *next_head; 
};

struct network {
    int vertex_count; 
    int edge_count;

    int total_cost;

    vertex_t source; 
    vertex_t sink; 

    struct edge *edges;
    struct edge **adj;
    struct edge **adjrev;

    struct edge **limits;
    struct edge *limit_sink;

    int *dist;
    struct edge **parent;
};

vertex_t
bellman_ford(struct network *network)
{
    for (int v = 0; v < network->vertex_count; v++) {
        network->parent[v] = NULL;
        network->dist[v] = INT_MAX;
    }

    network->dist[network->sink] = 0;

    vertex_t relaxed;
    for (int iter = 0; iter < network->vertex_count; iter++) {
        relaxed = -1;

        for (int e = 0; e < network->edge_count; e++) {
            struct edge *edge = &network->edges[e];

            bool can = (edge->capacity - edge->flow) > 0;

            int d = network->dist[edge->tail] + edge->cost;
            if (network->dist[edge->tail] != INT_MAX) {

                if (network->dist[edge->head] > d && can) {
                    network->dist[edge->head] = d;
                    network->parent[edge->head] = edge;

                    relaxed = edge->head;
                }
            }

            can = edge->flow > 0;
            if (network->dist[edge->head] != INT_MAX) {

                d = network->dist[edge->head] - edge->cost;
                if (network->dist[edge->tail] > d && can) {
                    network->dist[edge->tail] = d;
                    network->parent[edge->tail] = edge;

                    relaxed = edge->tail;
                }
            }
        }

        if (relaxed == -1) {
            break;
        }
    }

#ifdef DDEEBBUUGG__
    for (int v = 0; v < network->vertex_count; v++) {
        dprintf("%d ", network->dist[v]);
    }
    dprintf("\n");
#endif

    return relaxed;
}

void 
cancel_negative_cycles(struct network *network)
{
    vertex_t cycle_start;
    while ((cycle_start = bellman_ford(network)) != -1) {
        for (int i = 0; i < network->vertex_count; i++) {
            struct edge *edge = network->parent[cycle_start];
            cycle_start = edge->head == cycle_start ?
                edge->tail : edge->head;
        }

        vertex_t vertex = cycle_start;
        int flow = INT_MAX;

        do { // TODO  get rid of
            struct edge *edge = network->parent[vertex];

            int avail = (vertex == edge->head) ? 
                edge->capacity - edge->flow : edge->flow;

            flow = min(flow, avail);

            vertex = vertex == edge->head ? 
                edge->tail : edge->head;

        } while (vertex != cycle_start);


        //dprintf("flow = %d\n", flow);

        vertex = cycle_start;
        do {
            struct edge *edge = network->parent[vertex];

            int sgn = vertex == edge->tail ? -1 : 1;

            edge->flow += sgn * flow;
            network->total_cost += sgn * flow * edge->cost;

            vertex = vertex == edge->head ? 
                edge->tail : edge->head;
        } while (vertex != cycle_start);
    }
}


void 
pour_flow(struct network *network)
{
    vertex_t head_vertex = network->sink;
    int flow = network->dist[head_vertex];

    while (head_vertex != network->source) {
        struct edge *edge = network->parent[head_vertex];

        vertex_t neighbor = edge->head == head_vertex ? edge->tail : edge->head;
        int sgn = neighbor == edge->head ? -1 : 1;

        edge->flow += sgn * flow;
        network->total_cost += sgn * flow * edge->cost;


        head_vertex = neighbor;
    }
}

void maxflow_dfs(struct network *network, vertex_t vertex);

/* inline */ void 
maxflow_dfs_iter(struct network *network, vertex_t vertex, struct edge *edge)
{
#ifdef DDEEBBUUGG
    if (edge->tail != vertex && edge->head != vertex) {
        printf("EEEEEEE\n");
        exit(-1);
    }
#endif

    vertex_t neighbor = edge->tail == vertex ? edge->head : edge->tail;

    if (network->parent[neighbor] != NULL) {
        return;
    }

    int avail = (neighbor == edge->head) ? 
        edge->capacity - edge->flow : edge->flow;

    if (avail == 0) {
        return;
    }

    network->dist[neighbor] = min(avail, network->dist[vertex]);
    network->parent[neighbor] = edge;

    maxflow_dfs(network, neighbor);

}

void
maxflow_dfs(struct network *network, vertex_t vertex)
{
    for (struct edge *edge = network->adj[vertex]; edge != NULL;
            edge = edge->next_tail) {
        maxflow_dfs_iter(network, vertex, edge);

        if (network->parent[network->sink] != NULL) {
            return;
        }
    }

    for (struct edge *edge = network->adjrev[vertex]; edge != NULL;
            edge = edge->next_head) {
        maxflow_dfs_iter(network, vertex, edge);

        if (network->parent[network->sink] != NULL) {
            return;
        }
    }
}

void 
maxflow(struct network *network)
{

    network->total_cost = 0;

    for (int e = 0; e < network->edge_count; e++) {
        network->edges[e].flow = 0;
    }

    do {
        for (int v = 0; v < network->vertex_count; v++) {
            network->parent[v] = NULL;
            network->dist[v] = INT_MAX;
        }

        maxflow_dfs(network, network->source);
        if (network->parent[network->sink] != NULL) {
            pour_flow(network);
        }

    } while (network->parent[network->sink] != NULL);
}


void
mincost(struct network *network)
{
    maxflow(network);
    dprintf("pre cost = %d\n", network->total_cost);
    cancel_negative_cycles(network);
    dprintf("post cost = %d\n", network->total_cost);
}

struct edge *
add_edge(struct network *network, int *cursor,
         vertex_t tail, vertex_t head,
         int capacity, int cost)
{
    struct edge *edge = &network->edges[*cursor];

    edge->tail = tail;
    edge->head = head;
    edge->flow = 0;
    edge->cost = cost;
    edge->capacity = capacity;
    edge->next_tail = network->adj[tail];
    edge->next_head = network->adjrev[head];


    network->adj[tail] = edge;
    network->adjrev[head]= edge;

    (*cursor)++;

    return edge;
}

bool
solve_tournament()
{
    int player_count, budget;
    fscanf(stdin, "%d %d", &budget, &player_count);

    if (player_count <= 1) {
        return true;
    }

    int game_count = ((player_count * (player_count - 1)) / 2);
    int game_offset = 1;
    int player_offset = game_offset + game_count;

    struct network network;
    memset(&network, 0, sizeof(struct network));

    network.vertex_count = game_count + player_count + 3;
    network.edge_count = game_count * 3 + player_count + 1;

    /* alloc once ? */ 
    network.edges    = valloc(sizeof(struct edge) * network.edge_count);
    network.limits   = valloc(sizeof(struct edge *) * player_count);
    network.adj      = valloc(sizeof(struct edge *) * network.vertex_count);
    network.adjrev   = valloc(sizeof(struct edge *) * network.vertex_count);
    network.dist     = valloc(sizeof(int) * network.vertex_count);
    network.parent   = valloc(sizeof(struct edge *) * network.vertex_count);

#ifdef DDEEBBUUGG__
    dprintf("netwo\t = %p\n", &network);
    dprintf("edges\t = %p\n", network.edges);
    dprintf("limi \t = %p\n", network.limits);
    dprintf("adj  \t = %p\n", network.adj);
    dprintf("adjr \t = %p\n", network.adjrev);
    dprintf("dist \t = %p\n", network.dist);
    dprintf("pare \t = %p\n", network.parent);
#endif

    for (int v = 0; v < network.vertex_count; v++) {
        network.adj[v] = NULL;
        network.adjrev[v] = NULL;
    }

    vertex_t source_vertex = 0;
    vertex_t limit_vertex = player_offset + player_count;
    vertex_t sink_vertex = player_offset + player_count + 1;

    network.sink = sink_vertex;
    network.source = source_vertex;

#ifdef DDEEBBUUGG
    dotdebug("digraph { //c \n");
    dotdebug("\trankdir=LR;\n");

    dotdebug("\t%d [label=source];\n", source_vertex);
    dotdebug("\t%d [label=limit];\n", limit_vertex);
    dotdebug("\t%d [label=sink];\n", sink_vertex);
#endif

    int cursor = 0;
    int player_a, player_b, winner, loser, bribe;

    for (int game_idx = 0; game_idx < game_count; game_idx++) {
        fscanf(stdin, "%d %d %d %d", 
               &player_a, &player_b, 
               &winner, &bribe);
        
        loser = winner == player_a ? player_b : player_a;

        vertex_t game_vertex = game_offset + game_idx;
        vertex_t winner_vertex = player_offset + winner;
        vertex_t loser_vertex = player_offset + loser;


        add_edge(&network, &cursor,
                 source_vertex, game_vertex, 1, 0);

        add_edge(&network, &cursor,
                 game_vertex, winner_vertex, 1, 0);

        add_edge(&network, &cursor,
                 game_vertex, loser_vertex, 1, bribe);


        dotdebug("\t%d [label=\"%d vs %d\"];\n",
                game_vertex, player_a, player_b);
    }   

    vertex_t player_vertex;
    for (int player_idx = 0; player_idx < player_count; player_idx++) {
        player_vertex = player_offset + player_idx;

        dotdebug("\t%d [label=p%d]\n", player_vertex, player_idx);

        vertex_t head = player_idx == 0 ? sink_vertex : limit_vertex;
        network.limits[player_idx] = add_edge(
                &network, &cursor,
                player_vertex, head, 1, 0);

    }

    network.limit_sink = add_edge(&network, &cursor,
             limit_vertex, sink_vertex, 1, 0);


    dprintf("budget = %d\n", budget);
    bool found = false;
    for (int limit = player_count / 2; limit <= player_count - 1; limit++) {
        dprintf("\n");
        dprintf("limit = %d\n", limit);

        player_vertex = player_offset + 0;

        for (int player_idx = 0; player_idx < player_count; player_idx++) {
            network.limits[player_idx]->capacity = limit;
        }
        network.limit_sink->capacity = game_count - limit;

        mincost(&network);
        dprintf("spent = %d\n", network.total_cost);
        dprintf("\n");

        if (network.total_cost <= budget) {
            found = true;
            break;
        }
    }

#ifdef DDEEBBUUGG
    for (int e = 0; e < network.edge_count; e++) {
        struct edge *edge = &network.edges[e];

        dotdebug("\t%d -> %d [label=\"%d, %d, %d\"];\n", 
                 edge->tail, edge->head, 
                 edge->capacity, edge->cost, edge->flow);

    }
    dotdebug("}\n");
#endif

    free(network.adj);
    free(network.adjrev);

    free(network.dist);
    free(network.parent);

    free(network.limits);
    free(network.edges);

    return found;
}

int
main(int argc, const char *argv[])
{
#ifdef DDEEBBUUGG
    dot_file = fopen("out.dot", "w");
#endif

    int n;
    fscanf(stdin, "%d", &n);
    for (int i = 0; i < n; i++) {
        if (solve_tournament()) {
            printf("TAK\n");
        } else {
            printf("NIE\n");
        }
    }

#ifdef DDEEBBUUGG
    fclose(dot_file);
#endif
    return 0;
}


