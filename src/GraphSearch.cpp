#include <limits.h>
#include <unordered_set>
#include <unordered_map>
#include <stack>
#include <iostream>
#include "GraphSearch.h"
#include "Graph.h"
#include "Edge.h"
#include <limits.h>
#include "timer.h"
#include <vector>
#include <algorithm>
#include <omp.h>
#include "DataGraph.h"
#include <random>
#include "timer.h"
#include <iostream>
#include <fstream>

// #define SAVE_SEARCH_TREE
#define MAX_ST_CNT 1000

using namespace std;

vector<GraphMatch> GraphSearch::findAllSubgraphs(const Graph &g, const Graph &h, long long int limit)
{
    // If no criteria specified, just use the "dummy" criteria, that accepts everything.
    MatchCriteria criteria;
    return this->findAllSubgraphs(g, h, criteria, limit);
}

vector<GraphMatch> GraphSearch::findAllSubgraphs(const Graph &g, const Graph &h, const MatchCriteria &criteria, long long int limit)
{
    // Store class data structures
    _g = &g;
    _h = &h;
    _criteria = &criteria;

    bool debugOutput = false;

    // Stores the matching subgraphs as list of edge indices
    vector<GraphMatch> results;

    int n = _g->numNodes();
    int m = _g->numEdges();

    // Create lists of nodes that could be mapped to the given nodes
    vector<unordered_set<int>> h2gPossible = this->mapPossibleNodes();

    // Tables for mapping nodes and edges between the two graphs
    // -1 means no match has been assigned yet
    _h2gNodes.clear();
    _h2gNodes.resize(h.numNodes(), -1);
    _g2hNodes.clear();
    _g2hNodes.resize(n, -1);

    // Perform subgraph search, storing results along the way
    int numAssigned = 0;
    this->search(numAssigned, h2gPossible, results);

    return results;
}

vector<unordered_set<int>> GraphSearch::mapPossibleNodes()
{
    vector<unordered_set<int>> possible(_h->numNodes());

    int n = _g->numNodes();
    // Look at each vertex in H
    for (int h_v = 0; h_v < _h->numNodes(); h_v++)
    {
        // cout << "Testing possible nodes for " << h_v << endl;
        //  Check each vertex in G for a possible match
        for (int g_v = 0; g_v < n; g_v++)
        {
            // cout << "Trying " << g_v << endl;
            //  If it passes the criteria, than add it to the list of possible
            if (_criteria->isNodeMatch(*_g, g_v, *_h, h_v))
            {
                possible[h_v].insert(g_v);
            }
        }
    }
    return possible;
}

bool GraphSearch::search(int &numAssigned, vector<unordered_set<int>> &h2gPossible)
{
    // Test if nodes/edges match so far
    if (!matchesSoFar(numAssigned))
        return false;

    // Stop if we've reach the end
    if (numAssigned == _h->numNodes())
        return true;

    // Performs recursive DFS for matches
    int h_v = numAssigned;
    const auto &possible = h2gPossible[h_v];
    for (int g_v : possible)
    {
        if (_g2hNodes[g_v] < 0)
        {
            _h2gNodes[h_v] = g_v;
            _g2hNodes[g_v] = h_v;
            numAssigned++;
            if (search(numAssigned, h2gPossible))
                return true;
            _g2hNodes[g_v] = -1;
            _h2gNodes[h_v] = -1;
            numAssigned--;
        }
    }

    return false;
}

bool GraphSearch::search(int &numAssigned, vector<unordered_set<int>> &h2gPossible, vector<GraphMatch> &results)
{
    // Test if nodes/edges match so far
    if (!matchesSoFar(numAssigned))
        return false;

    // We've found a match if we've reached the end
    if (numAssigned == _h->numNodes())
    {
        // cout << "Found match!" << endl;
        //  Find the matching edges for the assignment
        // vector<int> matchingEdgeIndexes;
        GraphMatch matchingEdges;
        for (const Edge &hEdge : _h->edges())
        {
            int h_u = hEdge.source();
            int h_v = hEdge.dest();
            int g_u = _h2gNodes[h_u];
            int g_v = _h2gNodes[h_v];
            const vector<int> &gEdges = _g->getEdgeIndexes(g_u, g_v);
            for (int g_e : gEdges)
            {
                if (_criteria->isEdgeMatch(*_g, g_e, *_h, hEdge.index()))
                {
                    // matchingEdgeIndexes.push_back(g_e);
                    matchingEdges.addEdge(_g->edges()[g_e], hEdge);
                }
            }
        }
        results.push_back(matchingEdges);
        return true;
    }

    // Performs recursive DFS for matches
    int h_v = numAssigned;
    const auto &possible = h2gPossible[h_v];
    for (int g_v : possible)
    {
        if (_g2hNodes[g_v] < 0)
        {
            _h2gNodes[h_v] = g_v;
            _g2hNodes[g_v] = h_v;
            numAssigned++;
            search(numAssigned, h2gPossible, results);
            _g2hNodes[g_v] = -1;
            _h2gNodes[h_v] = -1;
            numAssigned--;
        }
    }

    return false;
}

bool GraphSearch::matchesSoFar(int numAssigned)
{
    // Check to see if every edge between the currently assigned vertices from
    // the subgraph is a matching edge in our graph
    for (const Edge &edge : _h->edges())
    {
        int h_u = edge.source();
        int h_v = edge.dest();
        if (h_u < numAssigned && h_v < numAssigned)
        {
            int g_u = _h2gNodes[h_u];
            int g_v = _h2gNodes[h_v];

            bool hasEdge = _g->hasEdge(g_u, g_v);
            if (hasEdge)
            {
                // Check to make sure they match the criteria
                const vector<int> edges = _g->getEdgeIndexes(g_u, g_v);
                for (int e : edges)
                {
                    if (_criteria->isEdgeMatch(*_g, e, *_h, edge.index()))
                    {
                        hasEdge = true;
                        break;
                    }
                }
            }
            if (!hasEdge)
            {
                return false;
                break;
            }
        }
    }
    return true;
}

long long int GraphSearch::findOrderedSubgraphs(const Graph &g, const Graph &h, long long int limit, int delta)
{
    // If no criteria specified, just use the "dummy" criteria, that accepts everything.
    MatchCriteria criteria;
    return findOrderedSubgraphs(g, h, criteria, limit, delta);
}

long long int GraphSearch::findOrderedSubgraphs(const Graph &g, const Graph &h, const MatchCriteria &criteria,
                                                long long int limit, int delta)
{
    // Store class data structures
    _g = &g;
    _h = &h;
    _criteria = &criteria;
    _delta = delta;

    bool debugOutput = false;

    // Stores the matching subgraphs as list of edge indices
    long long int results = 0;

    int n = _g->numNodes();
    int m = _g->numEdges();

    // List of all edge indexes
    _allEdges.resize(m);
    for (int i = 0; i < m; i++)
        _allEdges[i] = i;

    // Tables for mapping nodes and edges between the two graphs
    // -1 means no match has been assigned yet
    _h2gNodes.clear();
    _h2gNodes.resize(h.numNodes(), -1);
    _g2hNodes.clear();
    _g2hNodes.resize(n, -1);

    // Keeps track of the number of search edges mapped to a particular
    // node, so we can know if we need to reset its mapping when removing
    // edges from a search trail.
    _numSearchEdgesForNode.clear();
    _numSearchEdgesForNode.resize(n, 0);

    Timer t_findnext;

    // Stores all edges found that match our query.
    // Stack used to backtrack when a particular search ends up a dead-end.
    while (_sg_edgeStack.empty() == false) // Make sure it's empty to start
    {
        _sg_edgeStack.pop();
        //_h_edgeStack.pop();
    }

    // The edge from H we are trying to match in G
    int h_i = 0;
    // The current edge from G we are testing out (yes, should start at -1)
    int g_i = 0;

    // Loop until we can account for all subgraphs matching our edges
    while (true)
    {
        // Time of the last matched edge, for delta comparison
        // time_t lastEdgeTime = 0;
        // if(_sg_edgeStack.empty()==false)
        // lastEdgeTime = g.edges()[_sg_edgeStack.top()].time();

        // Time of current edge, for delta comparison
        time_t curEdgeTime = 0;
        if (g_i < m)
            curEdgeTime = g.edges()[g_i].time();

        // If we've run out of edges, we need to pop the last edge used,
        // and start the search back at the edge after that one
        while (g_i >= m || (_sg_edgeStack.empty() == false && g.edges()[g_i].time() - _firstEdgeTime > delta))
        {
            // If the edge stack is empty, then we have no options left
            // and need to give up.
            if (_sg_edgeStack.empty())
            {
                return results;
            }

            // Pop the stack
            int last_g_i = _sg_edgeStack.top();
            _sg_edgeStack.pop();

            // Get edge object
            const Edge &g_edge = _g->edges()[last_g_i];
            if (debugOutput)
                cout << "Giving up on edge " << last_g_i << ": " << g_edge.source() << ", " << g_edge.dest() << endl;

            // Decrement the number of edges for the nodes in g_i
            _numSearchEdgesForNode[g_edge.source()]--;
            _numSearchEdgesForNode[g_edge.dest()]--;

            // If any of them reach zero, then we need to remove the
            // node mapping for that node, since none of our edges are
            // currently using it (making it free to be re-assigned).
            if (_numSearchEdgesForNode[g_edge.source()] == 0)
            {
                int old_h_u = _g2hNodes[g_edge.source()];
                _h2gNodes[old_h_u] = -1;
                _g2hNodes[g_edge.source()] = -1;
            }
            if (_numSearchEdgesForNode[g_edge.dest()] == 0)
            {
                int old_h_v = _g2hNodes[g_edge.dest()];
                _h2gNodes[old_h_v] = -1;
                _g2hNodes[g_edge.dest()] = -1;
            }

            // Decrement h_i, so that we can find a new one
            h_i--;
            // Make sure we start the search immediately after the failed edge
            g_i = last_g_i + 1;

        }

        // Get query edge
        const Edge &h_edge = _h->edges()[h_i];
        int h_u = h_edge.source();
        int h_v = h_edge.dest();

        // Find matching edge, if possible
        // t_findnext.Start();
        g_i = this->findNextMatch(h_i, g_i);
        // t_findnext.Stop();

        if (g_i < m)
        {
            // Test to see if whole graph is found
            if (h_i + 1 == _h->numEdges())
            {
                // Create GraphMatch object
                // GraphMatch match = convert(_sg_edgeStack, g_i);
                // for(int i=0; i < match.edges().size(); i++)
                //     cout << _g->edges()[match.edges()[i]] << ",";
                // cout << endl;
                // Add new subgraph to the results
                // results.push_back(match);
                results++;
                // Don't increment h_i (or perform mappings), because we want
                // to find if there are other alternative subgraphs for that edge.
                // cout << "Found subgraph #" << results.size() << endl;
                // Test if we've reached our limit, and stop if we have.
                if (results >= limit)
                    return results;
            }
            // Otherwise, add the edge and mappings to the subgraph search
            // and continue on to find next edges.
            else
            {
                // Get matched edge
                const Edge &g_edge = _g->edges()[g_i];
                int g_u = g_edge.source();
                int g_v = g_edge.dest();

                // Set the first edge time, if needed
                if (_sg_edgeStack.empty())
                {
                    _firstEdgeTime = g_edge.time();
                    _firstEdgeid = g_edge.index();
                    _upper_time = _firstEdgeTime + _delta;
                }

                // Map the nodes from each graph
                _h2gNodes[h_u] = g_u;
                _h2gNodes[h_v] = g_v;
                _g2hNodes[g_u] = h_u;
                _g2hNodes[g_v] = h_v;

                // Increment number of search edges for each node in our G edge
                _numSearchEdgesForNode[g_u]++;
                _numSearchEdgesForNode[g_v]++;

                // Add it to the stack
                _sg_edgeStack.push(g_i);
                //_h_edgeStack.push(h_i);

                // Increment to next edge to find
                h_i++;
            }
        }
        // Increment the edge to test
        g_i++;
    }
    return results;
}

long long int GraphSearch::findOrderedSubgraphsMultiThread(const Graph &g, const Graph &h, const MatchCriteria &criteria,
                                                long long int limit, int delta, int num_of_threads, int partition_per_thread)
{
    long long int sum_results = 0;
    // prepare omp
    vector<GraphSearch *> searches;
    vector<long long int *> results;
    for (int i = 0; i < num_of_threads * partition_per_thread; i++)
    {
        searches.push_back(new GraphSearch());
        results.push_back(new long long int());
    }
    vector<int> start_index_vec;
    vector<int> end_index_vec;
    for (int i = 0; i < num_of_threads * partition_per_thread; i++)
    {
        int start_index = int(g.numEdges() * 1.0 * i / (num_of_threads * partition_per_thread));
        int end_index = int(g.numEdges() * 1.0 * (i + 1) / (num_of_threads * partition_per_thread));
        while (start_index > 0 && start_index < g.numEdges() && g.edges()[start_index - 1].source() == g.edges()[start_index - 1].dest())
        {
            start_index++;
        }
        while (end_index > 0 && end_index < g.numEdges() && g.edges()[end_index - 1].source() == g.edges()[end_index - 1].dest())
        {
            end_index++;
        }
        start_index_vec.push_back(start_index);
        end_index_vec.push_back(end_index);
    }
    double avg_time;
    #pragma omp parallel for schedule(dynamic, 1)
    for (int i = 0; i < num_of_threads * partition_per_thread; i++)
    {
        searches[i]->findOrderedSubgraphs(results[i], start_index_vec[i], end_index_vec[i], g, h, criteria, limit, delta);
    }
    for (int i = 0; i < num_of_threads * partition_per_thread; i++)
    {
        sum_results += *(results[i]);
    }
    return sum_results;
}

void GraphSearch::findOrderedSubgraphs(
    long long int *results, int start_edge_idx, int end_edge_idx,
    const Graph &g, const Graph &h, const MatchCriteria &criteria,
    long long int limit, int delta)
{
    // Store class data structures
    _g = &g;
    _h = &h;
    _criteria = &criteria;
    _delta = delta;

    bool debugOutput = false;

    // Stores the matching subgraphs as list of edge indices

    int n = _g->numNodes();
    int m = _g->numEdges();

    // List of all edge indexes
    _allEdges.resize(m);
    for (int i = 0; i < m; i++)
        _allEdges[i] = i;

    // Tables for mapping nodes and edges between the two graphs
    // -1 means no match has been assigned yet
    _h2gNodes.clear();
    _h2gNodes.resize(h.numNodes(), -1);
    _g2hNodes.clear();
    _g2hNodes.resize(n, -1);

    // Keeps track of the number of search edges mapped to a particular
    // node, so we can know if we need to reset its mapping when removing
    // edges from a search trail.
    _numSearchEdgesForNode.clear();
    _numSearchEdgesForNode.resize(n, 0);


    Timer t_findnext;

    // Stores all edges found that match our query.
    // Stack used to backtrack when a particular search ends up a dead-end.
    while (_sg_edgeStack.empty() == false) // Make sure it's empty to start
    {
        _sg_edgeStack.pop();
        //_h_edgeStack.pop();
    }

    // The edge from H we are trying to match in G
    int h_i = 0;
    // The current edge from G we are testing out (yes, should start at -1)
    int g_i = start_edge_idx;
    // Loop until we can account for all subgraphs matching our edges
    while (true)
    {
        // Time of the last matched edge, for delta comparison
        // time_t lastEdgeTime = 0;
        // if(_sg_edgeStack.empty()==false)
        // lastEdgeTime = g.edges()[_sg_edgeStack.top()].time();

        // Time of current edge, for delta comparison
        time_t curEdgeTime = 0;
        if (g_i < m)
            curEdgeTime = g.edges()[g_i].time();

        // If we've run out of edges, we need to pop the last edge used,
        // and start the search back at the edge after that one
        while (g_i >= m || _sg_edgeStack.empty() || (_sg_edgeStack.empty() == false && g.edges()[g_i].time() - _firstEdgeTime > delta))
        {
            // If the edge stack is empty, then we have no options left
            // and need to give up.
            if (_sg_edgeStack.empty())
            {
                if (g_i >= end_edge_idx)
                    return;
                else
                    break;
            }

            // Pop the stack
            int last_g_i = _sg_edgeStack.top();
            _sg_edgeStack.pop();

            // Get edge object
            const Edge &g_edge = _g->edges()[last_g_i];
            if (debugOutput)
                cout << "Giving up on edge " << last_g_i << ": " << g_edge.source() << ", " << g_edge.dest() << endl;

            // Decrement the number of edges for the nodes in g_i
            _numSearchEdgesForNode[g_edge.source()]--;
            _numSearchEdgesForNode[g_edge.dest()]--;

            // If any of them reach zero, then we need to remove the
            // node mapping for that node, since none of our edges are
            // currently using it (making it free to be re-assigned).
            if (_numSearchEdgesForNode[g_edge.source()] == 0)
            {
                int old_h_u = _g2hNodes[g_edge.source()];
                _h2gNodes[old_h_u] = -1;
                _g2hNodes[g_edge.source()] = -1;
            }
            if (_numSearchEdgesForNode[g_edge.dest()] == 0)
            {
                int old_h_v = _g2hNodes[g_edge.dest()];
                _h2gNodes[old_h_v] = -1;
                _g2hNodes[g_edge.dest()] = -1;
            }

            // Decrement h_i, so that we can find a new one
            h_i--;
            // Make sure we start the search immediately after the failed edge
            g_i = last_g_i + 1;
        }

        // Get query edge
        const Edge &h_edge = _h->edges()[h_i];
        int h_u = h_edge.source();
        int h_v = h_edge.dest();

        // Find matching edge, if possible
        // t_findnext.Start();
        g_i = this->findNextMatch(h_i, g_i);
        // t_findnext.Stop();

        if (g_i < m)
        {
            // Test to see if whole graph is found
            if (h_i + 1 == _h->numEdges())
            {
                // Convert stack to vector
                // vector<int> edges = convert(_sg_edgeStack);
                // Add the last edge to the list
                // edges.push_back(g_i);
                // Add new subgraph to the results
                // results.push_back(edges);

                // Create GraphMatch object
                // GraphMatch match = convert(_sg_edgeStack, g_i);
                // for(int i=0; i < match.edges().size(); i++)
                //     cout << match.edges()[i] << ",";
                // cout << endl;
                // Add new subgraph to the results
                // results.push_back(match);
                (*results)++;
                // Don't increment h_i (or perform mappings), because we want
                // to find if there are other alternative subgraphs for that edge.
                // cout << "Found subgraph #" << results.size() << endl;
                // Test if we've reached our limit, and stop if we have.
                if ((*results) >= limit)
                    return;
            }
            // Otherwise, add the edge and mappings to the subgraph search
            // and continue on to find next edges.
            else
            {
                // Get matched edge
                const Edge &g_edge = _g->edges()[g_i];
                int g_u = g_edge.source();
                int g_v = g_edge.dest();

                // Set the first edge time, if needed
                if (_sg_edgeStack.empty())
                {
                    _firstEdgeTime = g_edge.time();
                    _firstEdgeid = g_edge.index();
                    _upper_time = _firstEdgeTime + _delta;
                }

                // Map the nodes from each graph
                _h2gNodes[h_u] = g_u;
                _h2gNodes[h_v] = g_v;
                _g2hNodes[g_u] = h_u;
                _g2hNodes[g_v] = h_v;

                // Increment number of search edges for each node in our G edge
                _numSearchEdgesForNode[g_u]++;
                _numSearchEdgesForNode[g_v]++;

                // Add it to the stack
                _sg_edgeStack.push(g_i);
                //_h_edgeStack.push(h_i);

                // Increment to next edge to find
                h_i++;
            }
        }
        // Increment the edge to test
        g_i++;
    }
    return;
}

int GraphSearch::findNextMatch(int h_i, int g_i)
{
    bool debugOutput = false;

    // Get query edge
    const Edge &h_edge = _h->edges()[h_i];
    int h_u = h_edge.source();
    int h_v = h_edge.dest();

    // Default is to search over all edges starting at g_i
    const vector<int> *searchEdges = &_allEdges;

    // Look to see if nodes are already mapped, and just use those
    // node edges, if so. (Much faster!)
    if (_h2gNodes[h_u] >= 0 && _h2gNodes[h_v] >= 0)
    {
        const vector<int> &uEdges = _g->nodes()[_h2gNodes[h_u]].outEdges();
        const vector<int> &vEdges = _g->nodes()[_h2gNodes[h_v]].inEdges();
        if (uEdges.size() < vEdges.size())
            searchEdges = &uEdges;
        else
            searchEdges = &vEdges;
    }
    else if (_h2gNodes[h_u] >= 0)
    {
        searchEdges = &_g->nodes()[_h2gNodes[h_u]].outEdges();
    }
    else if (_h2gNodes[h_v] >= 0)
    {
        searchEdges = &_g->nodes()[_h2gNodes[h_v]].inEdges();
    }

    // Find starting place in the list
    int start = findStart(g_i, *searchEdges);

    // If no starting place can be found, just return that it's not possible
    if (start == searchEdges->size())
    {
        return _g->numEdges();
    }
    /*if(debugOutput)
    {
        cout << "Looking for an edge >= " << g_i << endl;
        cout << "Found this one: " <<
    }*/

    // Perform search
    int fnm = findNextMatch(h_i, *searchEdges, start);

    return fnm;
}

int GraphSearch::findStart(int g_i, const std::vector<int> &edgeIndexes)
{
    // If it's the original edges, just return g_i
    if (edgeIndexes.size() > g_i && edgeIndexes[g_i] == g_i)
        return g_i;

    // Test if any edge will work
    if (edgeIndexes.empty())
        return edgeIndexes.size();
    if (edgeIndexes.back() < g_i)
        return edgeIndexes.size();
    if (edgeIndexes.front() >= g_i)
        return 0;

    /*for(int i=0; i<edgeIndexes.size(); i++)
    {
        if(edgeIndexes[i] >= g_i)
            return i;
    }
    // If nothing was found, return the size of our list
    return edgeIndexes.size();*/

    // Otherwise, perform binary search
    int left = 0, right = edgeIndexes.size() - 1;
    while (true)
    {
        if (right <= left)
        {
            return left;
        }
        int i = (right + left) / 2;
        int ei = edgeIndexes[i];
        if (ei == g_i)
            return i;
        if (ei >= g_i && i == left)
            return i;
        if (ei < g_i)
            left = i + 1;
        else
        {
            if (edgeIndexes[i - 1] < g_i)
            {
                return i;
            }
            right = i - 1;
        }
    }
}

int GraphSearch::findNextMatch(int h_i, const std::vector<int> &edgesToSearch, int startIndex)
{
    bool debugOutput = false;

    // Get query edge
    const Edge &h_edge = _h->edges()[h_i];
    int h_u = h_edge.source();
    int h_v = h_edge.dest();

    // Check the time against the previous matched edge, if any exist
    bool checkTime = _sg_edgeStack.empty() == false;

    // Loop over all the edges to search
    for (int i = startIndex; i < edgesToSearch.size(); i++)
    {
        // Get the index of our edge in G
        int g_i = edgesToSearch[i];

        // Get original edge
        const Edge &g_edge = _g->edges()[g_i];
        int g_u = g_edge.source();
        int g_v = g_edge.dest();

        // If we've gone past our delta, stop the search
        // if(checkTime && g_edge.time() - _firstEdgeTime > _delta)
        if (checkTime && g_edge.time() > _upper_time)
        {
            return _g->numEdges();

            if (debugOutput)
            {
                cout << "Trying edge " << g_i << ": " << g_u << ", " << g_v << "    ";
                cout << "Need to match edge " << h_i << ": " << h_u << ", " << h_v << endl;
                cout << "   g[" << g_u << "]=" << _g2hNodes[g_u] << " g[" << g_v << "]=" << _g2hNodes[g_v] << endl;
                cout << "   h[" << h_u << "]=" << _h2gNodes[h_u] << " h[" << h_v << "]=" << _h2gNodes[h_v] << endl;
            }
        }
        // Make sure if the edge is a self-loop or not
        if ((h_u == h_v && g_u == g_v) || (h_u != h_v && g_u != g_v))
        {
            // Test if source nodes match, or both are unassigned
            if (_h2gNodes[h_u] == g_u || (_h2gNodes[h_u] < 0 && _g2hNodes[g_u] < 0))
            {
                // Test if destination nodes match, or both are unassigned
                if (_h2gNodes[h_v] == g_v || (_h2gNodes[h_v] < 0 && _g2hNodes[g_v] < 0))
                {
                    // Test if metadata criteria is a match
                    if (_criteria->isEdgeMatch(*_g, g_i, *_h, h_i))
                    {
                        if (debugOutput)
                            cout << "Edge " << g_i << ": " << g_u << ", " << g_v << " is a match" << endl;
                        return g_i;
                    }
                }
            }
        }
    }
    // If no match found, return the number of edges
    return _g->numEdges();
}

vector<int> GraphSearch::convert(stack<int> s)
{
    vector<int> v(s.size());
    for (int i = v.size() - 1; i >= 0; i--)
    {
        v[i] = s.top();
        s.pop();
    }
    return v;
}

GraphMatch GraphSearch::convert(const std::stack<int> &s, int g_lastEdge)
{
    GraphMatch gm;
    vector<int> gEdges = convert(s);
    gEdges.push_back(g_lastEdge);
    for (int h_i = 0; h_i < gEdges.size(); h_i++)
    {
        int g_i = gEdges[h_i];
        gm.addEdge(_g->edges()[g_i], _h->edges()[h_i]);
    }
    return gm;
}

/*Counting increasing 3-stars.*/
long long int GraphSearch::count_stars(int delta, int id_begin, int id_end)
{
    long long int star_count = 0;

    for(int i = id_begin; i < id_end; i++) /*Consider a temporal edge u->v*/
    {
        Edge e = _g->edges()[i];
        int e_src = e.source();
        int e_dst = e.dest();
        if(e_src == e_dst){
            continue;
        }
        const vector<int>& e_dst_out_edges = _g->nodes()[e_dst].outEdges(); /*A vector consisting of the id's of the edges directed away from v*/
        
        // e_dst_out_edges.index() > e.index()
        vector<int>::const_iterator e_dst_out_edges_start_it = upper_bound(           /*The starting point of edges directed away from v, with timestamp at least t(u->v)*/
            e_dst_out_edges.begin(), e_dst_out_edges.end(), e.index()
        );
        // e_dst_out_edges.time() <= e.time() + _delta
        vector<int>::const_iterator e_dst_out_edges_end_it = upper_bound(           /*The ending point of edges directed away from v, with timestamp at most t(u->v) + delta*/
            e_dst_out_edges.begin(), e_dst_out_edges.end(), e.time() + _delta,
            [&](const time_t &a, const int &b) { return a < _g->edges()[b].time(); }
        );

        long long int number_of_candidate_edges = 0;
        long long int number_of_pairs_on_same_edges = 0;
        map<int, int>edge_count_per_vertex;

        /*Creates a map, that for each dst vertex w of the outgoing edges v->w, stores the number of edges from v to w.*/
        for(vector<int>::const_iterator edge_iterator = e_dst_out_edges_start_it; edge_iterator != e_dst_out_edges_end_it; edge_iterator++)
        {
            if(_g->edges()[*edge_iterator].dest() == e_src || (_g->edges()[*edge_iterator].dest() == _g->edges()[*edge_iterator].source()))
                continue;
            else{
                edge_count_per_vertex[_g->edges()[*edge_iterator].dest()] += 1;
                number_of_candidate_edges += 1;
            }
        }

        /*Total number of pairs obtained from edges between same pair of nodes (v, w).*/
        for(auto itr = edge_count_per_vertex.begin(); itr != edge_count_per_vertex.end(); itr++)
            number_of_pairs_on_same_edges += (itr->second)*((itr->second) - 1)/2;

        /*Since you need to form pairs out of edges within the same list, no need to execute 'merge' subroutine. */
        long long int this_star_count = (((number_of_candidate_edges)*(number_of_candidate_edges - 1)/2) - number_of_pairs_on_same_edges);
        star_count += this_star_count;
    }
    return star_count;
}


long long int GraphSearch::count_stars_multi_thread(
    int delta, int num_of_threads, int partition_per_thread)
{
    long long int sum_star_count = 0;
    vector<long long int> star_counts(num_of_threads * partition_per_thread, 0);

    #pragma omp parallel for schedule(dynamic, 1)
    for (int i = 0; i < num_of_threads * partition_per_thread; i++)
    {
        int id_begin = _g->numEdges() / (num_of_threads * partition_per_thread) * i;
        int id_end =  min(_g->numEdges() / (num_of_threads * partition_per_thread) * (i+1), _g->numEdges());
        star_counts[i] = count_stars(delta, id_begin, id_end);
    }

    for (int i = 0; i < num_of_threads * partition_per_thread; i++)
    {
        sum_star_count += star_counts[i];
    }
    
    return sum_star_count;
}


bool GraphSearch::sample_directed_3path(
    int edge_id, vector<vector<vector<int>::const_iterator>>& sampling_neigh_edges_it,
    std::vector<Edge>& sampled_edges, vector<long long int>& failure)
{
    unsigned int seed = time(NULL) ^ omp_get_thread_num();
    sampled_edges.clear();
    // sample edge (u,v) with probability (in_d_u) * (out_d_v)
    // int edge_id = sample_weight(gen);
    // if(edge_id == 453617 || edge_id == 577917)
    //     cout << "edge_id:" << edge_id << endl;
    Edge u_v_edge = _g->edges()[edge_id];
    int u = u_v_edge.source();
    int v = u_v_edge.dest();
    // if (u == v) return 0; // already checked by preprocess_sampling_weights()
    sampled_edges.push_back(u_v_edge);
    // sample u'->u edge from all in edges of u within delta timestamp constraints
    int num_u_in_edges = distance(sampling_neigh_edges_it[edge_id][0], sampling_neigh_edges_it[edge_id][1]);
    if (num_u_in_edges == 0)
        return false;
    int u_in_edges_randidx = rand_r(&seed) % num_u_in_edges;
    int u_in_edge_id = *(sampling_neigh_edges_it[edge_id][0] + u_in_edges_randidx);
    // check if u'->u edge meets requirement
    Edge u_in_edge = _g->edges()[u_in_edge_id];
    int u_prime = u_in_edge.source();
    // u' and v must be different, u'->u edge time must < u->v time
    if ((u_prime == v) || (u_prime == u))
    {
        failure[0]++;
        return false;
    }
    sampled_edges.push_back(u_in_edge);
    // sample v->v' edge from all out edges of v within delta timestamp constraints
    int num_v_out_edges = distance(sampling_neigh_edges_it[edge_id][2], sampling_neigh_edges_it[edge_id][3]);
    if (num_v_out_edges == 0)
        return false;
    int v_out_edge_randidx = rand_r(&seed) % num_v_out_edges;
    int v_out_edge_id = *(sampling_neigh_edges_it[edge_id][2] + v_out_edge_randidx);
    // v' and u must be different
    Edge v_out_edge = _g->edges()[v_out_edge_id];
    int v_prime = v_out_edge.dest();
    if ((v_prime == u) || (v_prime == v) || (v_prime == u_prime))
    {
        failure[0]++;
        return false;
    }
    // delta constraints
    if (v_out_edge.time() > u_in_edge.time() + _delta)
    {
        failure[1]++;
        return false;
    }
    sampled_edges.push_back(v_out_edge);
    return true;
}

vector<long long int> GraphSearch::check_motif(vector<Edge> &sampled_edges)
{
    vector<long long int> motifs_cnts(8, 0);
    Edge u_v_edge = sampled_edges[0];
    Edge u_in_edge = sampled_edges[1];
    Edge v_out_edge = sampled_edges[2];
    int u = u_v_edge.source();
    int v = u_v_edge.dest();
    int u_prime = u_in_edge.source();
    int v_prime = v_out_edge.dest();

    // M1: 3-path
    motifs_cnts[1] = 1;
    int v_prime_u_edges_num = 0;

    vector<int>::iterator v_prime_u_edges_left_it;
    vector<int>::iterator v_prime_u_edges_right_it;
    vector<int>::iterator v_prime_u_prime_edges_left_it;
    vector<int>::iterator v_prime_u_prime_edges_right_it;
    vector<int>::iterator u_prime_v_edges_left_it;
    vector<int>::iterator u_prime_v_edges_right_it;
    vector<int>::iterator u_u_prime_edges_left_it;
    vector<int>::iterator u_u_prime_edges_right_it;
    bool v_prime_u_exist = false;
    bool v_prime_u_prime_exist = false;
    bool u_prime_v_exist = false;
    bool u_u_prime_exist = false;

    // M2: tailed triangle v'->u
    if (_g->nodeEdges().find(v_prime) != _g->nodeEdges().end())
    {
        if(_g->nodeEdges()[v_prime].find(u) != _g->nodeEdges()[v_prime].end())
        {
            v_prime_u_exist = true;
            vector<int> &v_prime_u_edges_ids = _g->nodeEdges()[v_prime][u];
            // find idx > v_out_edge.index()
            v_prime_u_edges_left_it = upper_bound(
                v_prime_u_edges_ids.begin(), v_prime_u_edges_ids.end(), v_out_edge.index()
            );
            // find timestamp <= u_in_edge.time() + delta
            v_prime_u_edges_right_it = upper_bound(
                v_prime_u_edges_ids.begin(), v_prime_u_edges_ids.end(), u_in_edge.time() + _delta,
                [&](const time_t &a, const int &b) { return a < _g->edges()[b].time(); }
            );
            v_prime_u_edges_num = distance(v_prime_u_edges_left_it, v_prime_u_edges_right_it);
            if (v_prime_u_edges_num > 0) {
                motifs_cnts[2] = v_prime_u_edges_num;
            }
        }
    }
    // M3: four cycle_1 v'->u'
    if (_g->nodeEdges().find(v_prime) != _g->nodeEdges().end())
    {
        if(_g->nodeEdges()[v_prime].find(u_prime) != _g->nodeEdges()[v_prime].end())
        {
            v_prime_u_prime_exist = true;
            vector<int> &v_prime_u_prime_edges_ids = _g->nodeEdges()[v_prime][u_prime];
            // find idx > v_out_edge.index()
            v_prime_u_prime_edges_left_it = upper_bound(
                v_prime_u_prime_edges_ids.begin(), v_prime_u_prime_edges_ids.end(), v_out_edge.index()
            );
            // find timestamp <= u_v_edge.time() + _delta
            v_prime_u_prime_edges_right_it = upper_bound(
                v_prime_u_prime_edges_ids.begin(), v_prime_u_prime_edges_ids.end(), u_in_edge.time() + _delta,
                [&](const time_t &a, const int &b) { return a < _g->edges()[b].time(); }
            );
            int v_prime_u_prime_edges_num = distance(v_prime_u_prime_edges_left_it, v_prime_u_prime_edges_right_it);
            if (v_prime_u_prime_edges_num > 0){
                motifs_cnts[3] = v_prime_u_prime_edges_num;
            }
        }
    }

    // M4: 4 cyecle_2 v'->u', u->u'
    // find number of pair (e3, e4) that e3.time() <= e4.time()
    // e3 in (v_prime_u_prime_edges_left_it, v_prime_u_prime_edges_right_it)
    // e4 in (u_u_prime_edges_left_it, u_u_prime_edges_right_it)
    if (_g->nodeEdges().find(u) != _g->nodeEdges().end())
    {
        if(_g->nodeEdges()[u].find(u_prime) != _g->nodeEdges()[u].end())
        {
            u_u_prime_exist = true;
            vector<int> &u_u_prime_edges_ids = _g->nodeEdges()[u][u_prime];
            // find idx > v_out_edge.index()
            u_u_prime_edges_left_it = upper_bound(
                u_u_prime_edges_ids.begin(), u_u_prime_edges_ids.end(), v_out_edge.index()
            );
            // find timestamp <= u_in_edge.time() + delta
            u_u_prime_edges_right_it = upper_bound(
                u_u_prime_edges_ids.begin(), u_u_prime_edges_ids.end(), u_in_edge.time() + _delta,
                [&](const time_t &a, const int &b) { return a < _g->edges()[b].time(); }
            );
        }
    }
    int num_M4 = 0;
    if (v_prime_u_prime_exist && u_u_prime_exist)
    {
        vector<int>::iterator v_prime_u_prime_edges_it = v_prime_u_prime_edges_left_it;
        vector<int>::iterator u_u_prime_edges_it = u_u_prime_edges_left_it;
        while(
            (v_prime_u_prime_edges_it != v_prime_u_prime_edges_right_it) &&
            (u_u_prime_edges_it != u_u_prime_edges_right_it)
        )
        {
            // keep moving u_u_prime_edges_it to right if v'u'.time() > uu'.time()
            if (_g->edges()[*v_prime_u_prime_edges_it].time() > _g->edges()[*u_u_prime_edges_it].time())
            {
                u_u_prime_edges_it++;
            }
            // it is gauranteened that v'u'.time() <= uu'.time()
            // update count and increment v_prime_u_edges_it
            else
            {
                num_M4 += distance(u_u_prime_edges_it, u_u_prime_edges_right_it);
                v_prime_u_prime_edges_it++;
            }
        }
    }
    motifs_cnts[4] = num_M4;
    
    // M5: chordal-4-cycle_1 v'->u, v'->u'
    // find number of pair(e3, e4) that e3.time() <= e4.time() 
    // e3 in (v_prime_u_edges_left_it, v_prime_u_edges_right_it)
    // e4 in (v_prime_u_prime_edges_left_it, v_prime_u_prime_edges_right_it)
    int num_chordal_4_cycle = 0;
    if(v_prime_u_exist && v_prime_u_prime_exist)
    {
        vector<int>::iterator v_prime_u_edges_it = v_prime_u_edges_left_it;
        vector<int>::iterator v_prime_u_prime_edges_it = v_prime_u_prime_edges_left_it;
        while(
            (v_prime_u_edges_it != v_prime_u_edges_right_it) &&
            (v_prime_u_prime_edges_it != v_prime_u_prime_edges_right_it) 
        )
        {
            // keep moving v_prime_u_prime_edges_it to right if v'u.time() > v'u'.time()
            if(_g->edges()[*v_prime_u_edges_it].time() > _g->edges()[*v_prime_u_prime_edges_it].time())
            {
                v_prime_u_prime_edges_it++;
            }
            // it is gauranteened that v'u.time() <= v'u'.time()
            // update count and increment v_prime_u_edges_it
            else
            {
                num_chordal_4_cycle += distance(v_prime_u_prime_edges_it, v_prime_u_prime_edges_right_it);
                v_prime_u_edges_it++;
            }
        }
    }
    motifs_cnts[5] = num_chordal_4_cycle;

    // M6: chordal-4-cycle_2 v'->u, v'->u', u->u'
    int num_M6 = 0;
    if (v_prime_u_exist && v_prime_u_prime_exist && u_u_prime_exist)
    {
        vector<int>::iterator v_prime_u_edges_it = v_prime_u_edges_left_it;
        // vector<int>::iterator v_prime_u_prime_edges_it = v_prime_u_prime_edges_left_it;
        vector<int>::iterator u_u_prime_edges_it = u_u_prime_edges_left_it;
        for (vector<int>::iterator v_prime_u_prime_edges_it = v_prime_u_prime_edges_left_it; v_prime_u_prime_edges_it != v_prime_u_prime_edges_right_it; v_prime_u_prime_edges_it++)
        {
            while(v_prime_u_edges_it != v_prime_u_edges_right_it && _g->edges()[*v_prime_u_edges_it].time() <= _g->edges()[*v_prime_u_prime_edges_it].time())
            {
                v_prime_u_edges_it++;
            }
            while(u_u_prime_edges_it != u_u_prime_edges_right_it && _g->edges()[*u_u_prime_edges_it].time() < _g->edges()[*v_prime_u_prime_edges_it].time())
            {
                u_u_prime_edges_it++;
            }
            int num_v_prime_u = distance(v_prime_u_edges_left_it, v_prime_u_edges_it);
            int num_u_u_prime = distance(u_u_prime_edges_it, u_u_prime_edges_right_it);
            num_M6 += num_v_prime_u * num_u_u_prime;
        }
    }
    motifs_cnts[6] = num_M6;

    // M7: 4-clique
    int num_4_clique = 0;
    // u'->v
    if (_g->nodeEdges().find(u_prime) != _g->nodeEdges().end())
    {
        if(_g->nodeEdges()[u_prime].find(v) != _g->nodeEdges()[u_prime].end())
        {
            u_prime_v_exist = true;
            vector<int> &u_prime_v_edges_ids = _g->nodeEdges()[u_prime][v];
            // find idx > v_out_edge.index()
            u_prime_v_edges_left_it = upper_bound(
                u_prime_v_edges_ids.begin(), u_prime_v_edges_ids.end(), v_out_edge.index()
            );
            // find timestamp <= u_v_edge.time() + _delta
            u_prime_v_edges_right_it = upper_bound(
                u_prime_v_edges_ids.begin(), u_prime_v_edges_ids.end(), u_in_edge.time() + _delta,
                [&](const time_t &a, const int &b) { return a < _g->edges()[b].time(); }
            );
        }
    }
    
    if(v_prime_u_exist && v_prime_u_prime_exist && u_prime_v_exist)
    {
        vector<int>::iterator v_prime_u_edges_it = v_prime_u_edges_left_it;
        vector<int>::iterator v_prime_u_prime_edges_it = v_prime_u_prime_edges_left_it;
        vector<int>::iterator u_prime_v_edges_it = u_prime_v_edges_left_it;
        for (vector<int>::iterator v_prime_u_prime_edges_it = v_prime_u_prime_edges_left_it; v_prime_u_prime_edges_it != v_prime_u_prime_edges_right_it; v_prime_u_prime_edges_it++)
        {
            while(v_prime_u_edges_it != v_prime_u_edges_right_it && _g->edges()[*v_prime_u_edges_it].time() <= _g->edges()[*v_prime_u_prime_edges_it].time())
            {
                v_prime_u_edges_it++;
            }
            while(u_prime_v_edges_it != u_prime_v_edges_right_it && _g->edges()[*u_prime_v_edges_it].time() < _g->edges()[*v_prime_u_prime_edges_it].time())
            {
                u_prime_v_edges_it++;
            }
            int num_v_prime_u = distance(v_prime_u_edges_left_it, v_prime_u_edges_it);
            int num_u_prime_v = distance(u_prime_v_edges_it, u_prime_v_edges_right_it);
            // cout << "num_v_prime_u * num_u_prime_v: " << num_v_prime_u * num_u_prime_v<< endl;
            num_4_clique +=  num_v_prime_u * num_u_prime_v;
        }
    }
    motifs_cnts[7] = num_4_clique;
    return motifs_cnts;
}


vector<float> GraphSearch::estimate_motif(const vector<long long int> &motifs_cnts, int num_sample, long long int W)
{
    vector<float> estimated_cnts(motifs_cnts.size(), 0);
    estimated_cnts[0] = (float) motifs_cnts[0]; // exact 3-star
    float_t W_div_k = (float_t) W / num_sample;
    for(size_t i=1; i < motifs_cnts.size(); i++)
    {
        estimated_cnts[i] = (float) motifs_cnts[i]  * W_div_k;
    }
    return estimated_cnts;
}


bool GraphSearch::out_of_range(time_t target, time_t left, time_t right)
{
    return (target < left) || (target > right);
}

long long int GraphSearch::preprocess_sampling_weights(
    vector<int>& sampling_weights, vector<vector<vector<int>::const_iterator>>& sampling_neigh_edges_it)
{
    long long int W = 0;

    #pragma omp parallel for reduction(+:W)
    for(int i=0; i < _g->numEdges(); i++)
    {
        Edge e = _g->edges()[i];
        int e_src = e.source();
        int e_dst = e.dest();
        if(e_src == e_dst){
            sampling_weights[i] = 0;
            continue;
        }
        const vector<int>& e_src_in_edges = _g->nodes()[e_src].inEdges();
        const vector<int>& e_dst_out_edges = _g->nodes()[e_dst].outEdges();
        // e_src_in_edges.index() < e.index()
        vector<int>::const_iterator e_src_in_edges_right_it = lower_bound(
            e_src_in_edges.begin(), e_src_in_edges.end(), e.index()
        );
        // e_src_in_edges.time() >= e.time() - _delta
        vector<int>::const_iterator e_src_in_edges_left_it = lower_bound(
            e_src_in_edges.begin(), e_src_in_edges.end(), e.time() - _delta,
            [&](const int &a, const time_t &b) { return _g->edges()[a].time() < b; }
        );
        int num_e_src_in_edges = distance(e_src_in_edges_left_it, e_src_in_edges_right_it);
        
        // e_dst_out_edges.index() > e.index()
        vector<int>::const_iterator e_dst_out_edges_left_it = upper_bound(
            e_dst_out_edges.begin(), e_dst_out_edges.end(), e.index()
        );
        // e_dst_out_edges.time() <= e.time() - _delta
        vector<int>::const_iterator e_dst_out_edges_right_it = upper_bound(
            e_dst_out_edges.begin(), e_dst_out_edges.end(), e.time() + _delta,
            [&](const time_t &a, const int &b) { return a < _g->edges()[b].time(); }
        );
        int num_e_dst_out_edges = distance(e_dst_out_edges_left_it, e_dst_out_edges_right_it);
        
        sampling_weights[i] = num_e_src_in_edges * num_e_dst_out_edges;
        sampling_neigh_edges_it[i] = {
            e_src_in_edges_left_it, e_src_in_edges_right_it,
            e_dst_out_edges_left_it, e_dst_out_edges_right_it
        };
        W += sampling_weights[i];
    }
    return W;
}

vector<long long int> GraphSearch::sampleAndCheckMotif(
    long long int max_trial, discrete_distribution<>& edge_in_mult_out_weights,
    vector<vector<vector<int>::const_iterator>>& sampling_neigh_edges_it)
{
    random_device rd;
    mt19937 eng(rd() ^ omp_get_thread_num());

    vector<long long int> failure(2,0);
    vector<long long int> motifs_cnts(8, 0);

    for(long long int trial=0; trial < max_trial; trial++)
    {
        vector<Edge> sampled_edges(3, Edge(-1,-1,-1,-1));
        int edge_id = edge_in_mult_out_weights(eng);
        bool found = sample_directed_3path(edge_id, sampling_neigh_edges_it, sampled_edges, failure);
        if(!found)
        {
            continue;
        }
        vector<long long int> this_motifs_cnts = check_motif(sampled_edges);
        for(int i = 1; i < 8; i++)
        {
            motifs_cnts[i] += this_motifs_cnts[i];
        }
    }

    return motifs_cnts;
}

vector<float> GraphSearch::threePathSample(const Graph &g,
                                                int num_of_threads, int partition_per_thread,
                                                int delta, long long int max_trial)
{
    // Store class data structures
    _g = &g;
    _delta = delta;

    bool debugOutput = false;

    // Stores the matching subgraphs as list of edge indices
    long long int results = 0;
    // long long int results = 0;

    int n = _g->numNodes();
    int m = _g->numEdges();

    long long int success = 0;
    // long long int trial = 0;
    long long int W = 0;

    // motifs counts: 8 motifs (3-star don't care)
    // M1: 3-star, M2: 3-path, M3: tailed_traignle,
    // M4: 4-cycle_1, M5: 4-cycle_2, M6: chordal-4-cycle_1,
    // M7: chordal-4-cycle_2, M8:4-clique
    vector<long long int> motifs_cnts(8, 0);

    vector<int> sampling_weights(_g->numEdges());
    vector<vector<vector<int>::const_iterator>> sampling_neigh_edges_it(_g->numEdges());

    W = preprocess_sampling_weights(sampling_weights, sampling_neigh_edges_it);

    discrete_distribution<> edge_in_mult_out_weights(sampling_weights.begin(), sampling_weights.end());

    if (num_of_threads > 1)
    {
        // prepare omp
        // trial number for each thread and partition
        long long int single_trial_num = (max_trial + num_of_threads * partition_per_thread - 1) / (num_of_threads * partition_per_thread);
        // the last trial may has less trial_num than others
        long long int last_trial_num = max_trial - single_trial_num * (num_of_threads * partition_per_thread - 1);
        vector<vector<long long int>> trial_motifs_cnts(num_of_threads * partition_per_thread, vector<long long int>{});
        // trial_motifs_cnts[i][j] , i the parition i, j is the jth motif count

        #pragma omp parallel for schedule(dynamic, 1)
        for (int i = 0; i < num_of_threads * partition_per_thread; i++)
        {
            int trial_num = single_trial_num;
            if (i == num_of_threads * partition_per_thread - 1)
                trial_num = last_trial_num;
            trial_motifs_cnts[i] = (sampleAndCheckMotif(trial_num, edge_in_mult_out_weights, sampling_neigh_edges_it));
        }

        
        for(int i = 1; i < 8; i++)
        {
            for (int j = 0; j < num_of_threads * partition_per_thread; j++)
            {
                motifs_cnts[i] += trial_motifs_cnts[j][i];
            }
        }
    }
    else //single-thread
    {
        motifs_cnts = sampleAndCheckMotif(max_trial, edge_in_mult_out_weights, sampling_neigh_edges_it);
    }

    vector<float> estimted_cnts = estimate_motif(motifs_cnts, max_trial, W);

    return estimted_cnts;
}


long long int GraphSearch::count3Star(
    const Graph &g, int num_of_threads, int partition_per_thread, int delta)
{
    // Store class data structures
    _g = &g;
    _delta = delta;

    int n = _g->numNodes();
    int m = _g->numEdges();

    long long int results = 0;
    
    if(num_of_threads > 1)
        results = count_stars_multi_thread(delta, num_of_threads, partition_per_thread);
    else
        results = count_stars(delta, 0, m);

    return results;
}

bool GraphSearch::sample112(
    int iter,
    mt19937& eng,
    discrete_distribution<>& e2_weights_distr,
    vector<long long int>& e1_sampling_weights,
    vector<long long int>& e3_sampling_weights,
    vector<Edge>& sampled_edges)
{
    unsigned int seed = time(NULL) ^ omp_get_thread_num() ^ static_cast<unsigned int>(iter);
    sampled_edges.clear();

    /* Sample e2  using e2_weights_distr */
    int e2_id = e2_weights_distr(eng);
    // cout << "e2_id: " << e2_id << endl;
    Edge e2 = _g->edges()[e2_id];
    int u = e2.source();
    int v = e2.dest();
    // if (u == v) return 0; // already checked by preprocess_sampling_weights()
    sampled_edges[2] = e2;
    
    /* Sample e1 in [t_e2 - delta, t_e2] from inEdges(u) using e1_sampling_weights */
    // u_in_edges.index() < e2.index()
    const vector<int>& u_in_edges = _g->nodes()[u].inEdges();
    vector<int>::const_iterator u_in_edges_right_it = lower_bound(
        u_in_edges.begin(), u_in_edges.end(), e2.index()
    );
    // u_in_edges.time() >= e2.time() - _delta
    vector<int>::const_iterator u_in_edges_left_it = lower_bound(
        u_in_edges.begin(), u_in_edges.end(), e2.time() - _delta,
        [&](const int &a, const time_t &b) { return _g->edges()[a].time() < b; }
    );
    // sample u'->u edge from all in edges of u within delta timestamp constraints
    int num_u_in_edges = distance(u_in_edges_left_it, u_in_edges_right_it);
    if (num_u_in_edges == 0)
        return false;
    vector<int> cur_e1_weights(num_u_in_edges, 0);
    for(int i = 0; i < cur_e1_weights.size(); i++)
    {
        cur_e1_weights[i] = e1_sampling_weights[*(u_in_edges_left_it+i)];
    }
    discrete_distribution<> e1_weights_distr(cur_e1_weights.begin(), cur_e1_weights.end());
    int e1_id = *(u_in_edges_left_it + e1_weights_distr(eng));
    // cout << "e1_id: " << e1_id << endl;
    Edge e1 = _g->edges()[e1_id];
    int u_prime = e1.source();
    if ((u_prime == u) || (u_prime == v))
        return false;
    if (e1_id > e2_id || e1.time() < e2.time() - _delta)
        return false;
    sampled_edges[1] = e1;

    /* Sample e0 from [t_e1 - delta, t_e1] from inEdges(u') uniformly*/
    // u_prime_in_edges.index() < e1.index()
    const vector<int>& u_prime_in_edges = _g->nodes()[u_prime].inEdges();
    vector<int>::const_iterator u_prime_in_edges_right_it = lower_bound(
        u_prime_in_edges.begin(), u_prime_in_edges.end(), e1.index()
    );
    // u_prime_in_edges.time() >= e1.time() - _delta
    vector<int>::const_iterator u_prime_in_edges_left_it = lower_bound(
        u_prime_in_edges.begin(), u_prime_in_edges.end(), e1.time() - _delta,
        [&](const int &a, const time_t &b) { return _g->edges()[a].time() < b; }
    );
    int num_u_prime_in_edges = distance(u_prime_in_edges_left_it, u_prime_in_edges_right_it);
    if (num_u_prime_in_edges == 0)
        return false;
    int u_prime_in_edges_randidx = rand_r(&seed) % num_u_prime_in_edges;
    int e0_id = *(u_prime_in_edges_left_it + u_prime_in_edges_randidx);
    // cout << "e0_id: " << e0_id << endl;
    // check if u'->u edge meets requirement
    Edge e0 = _g->edges()[e0_id];
    int w = e0.source();
    // u' and v must be different, u'->u edge time must < u->v time
    if ((w == v) || (w == u) || (w == u_prime))
    {
        return false;
    }
    if (e0.time() < e2.time() - _delta)
        return false;
    sampled_edges[0] = e0;

    // /* Sample e3 in [t_e2, t_e2 + delta] from outEdges(v) using e3_sampling_weights */
    const vector<int>& v_out_edges = _g->nodes()[v].outEdges();
    // v_out_edges.index() > e2.index()
    vector<int>::const_iterator v_out_edges_left_it = upper_bound(
        v_out_edges.begin(), v_out_edges.end(), e2.index()
    );
    // v_out_edges.time() <= e2.time() + _delta
    vector<int>::const_iterator v_out_edges_right_it = upper_bound(
        v_out_edges.begin(), v_out_edges.end(), e2.time() + _delta,
        [&](const time_t &a, const int &b) { return a < _g->edges()[b].time(); }
    );
    // sample v->v' edge from all out edges of v within delta timestamp constraints
    int num_v_out_edges = distance(v_out_edges_left_it, v_out_edges_right_it);
    if (num_v_out_edges == 0)
        return false;
    vector<int> cur_e3_weights(num_v_out_edges, 0);
    for(int i = 0; i < cur_e3_weights.size(); i++)
    {
        cur_e3_weights[i] = e3_sampling_weights[*(v_out_edges_left_it + i)];
    }
    discrete_distribution<> e3_weights_distr(cur_e3_weights.begin(), cur_e3_weights.end());
    int e3_id = *(v_out_edges_left_it + e3_weights_distr(eng));
    // cout << "e3_id: " << e3_id << endl;
    Edge e3 = _g->edges()[e3_id];
    int v_prime = e3.dest();
    if ((v_prime == u) || (v_prime == v) || (v_prime == u_prime) || (v_prime == w))
        return false;
    if (e3.time() > e0.time() + _delta)
        return false;
    sampled_edges[3] = e3;

    /* Sample e4 from [t_e3, t_e3 + delta] from inEdges(u') uniformly*/
    const vector<int>& v_prime_out_edges = _g->nodes()[v_prime].outEdges();
    // v_prime_out_edges.index() > e3.index()
    vector<int>::const_iterator v_prime_out_edges_left_it = upper_bound(
        v_prime_out_edges.begin(), v_prime_out_edges.end(), e3.index()
    );
    // v_prime_out_edges.time() <= e3.time() + _delta
    vector<int>::const_iterator v_prime_out_edges_right_it = upper_bound(
        v_prime_out_edges.begin(), v_prime_out_edges.end(), e3.time() + _delta,
        [&](const time_t &a, const int &b) { return a < _g->edges()[b].time(); }
    );
    int num_v_prime_out_edges = distance(v_prime_out_edges_left_it, v_prime_out_edges_right_it);
    if (num_v_prime_out_edges == 0)
        return false;
    int v_prime_out_edges_randidx = rand_r(&seed) % num_v_prime_out_edges;
    int e4_id = *(v_prime_out_edges_left_it + v_prime_out_edges_randidx);
    // cout << "e4_id: " << e4_id << endl;
    // check if u'->u edge meets requirement
    Edge e4 = _g->edges()[e4_id];
    int s = e4.dest();
    if ((s == v) || (s == u) || (s == u_prime) || (s == w) || (s == v_prime))
    {
        return false;
    }
    if (e4.time() > e0.time() + _delta)
        return false;
    sampled_edges[4] = e4;
    
    return true;
}

vector<long long int> GraphSearch::check_motif112(vector<Edge> &sampled_edges)
{
    vector<long long int> motifs_cnts(2, 0);
    Edge u_v_edge = sampled_edges[2];
    Edge u_in_edge = sampled_edges[1];
    Edge v_out_edge = sampled_edges[3];
    Edge w_u_prime_edge = sampled_edges[0];
    Edge v_prime_s_edge = sampled_edges[4];
    int u = u_v_edge.source();
    int v = u_v_edge.dest();
    int u_prime = u_in_edge.source();
    int v_prime = v_out_edge.dest();
    int w = w_u_prime_edge.source();
    int s = v_prime_s_edge.dest();

    bool u_w_exist = false;
    vector<int>::iterator w_u_edges_left_it;
    vector<int>::iterator w_u_edges_right_it;

    // M1: 3-path
    motifs_cnts[0] = 1;

    // M2: 3-tailed triangle
    // u->w t0
    if (_g->nodeEdges().find(u) != _g->nodeEdges().end())
    {
        if(_g->nodeEdges()[u].find(w) != _g->nodeEdges()[u].end())
        {
            u_w_exist = true;
            vector<int> &u_w_edges_ids = _g->nodeEdges()[u][w];
            // find idx < w_u_prime_edge.index()
            w_u_edges_right_it = lower_bound(
                u_w_edges_ids.begin(), u_w_edges_ids.end(), w_u_prime_edge.index()
            );
            // find timestamp >= v_prime_s_edge.time() - _delta
            w_u_edges_left_it = lower_bound(
                u_w_edges_ids.begin(), u_w_edges_ids.end(), v_prime_s_edge.time() - _delta,
                [&](const int &a, const time_t &b) { return _g->edges()[a].time() < b; }
            );
            int u_w_edges_num = distance(w_u_edges_left_it, w_u_edges_right_it);
            if (u_w_edges_num > 0) {
                motifs_cnts[1] = u_w_edges_num;
            }
        }
    }
            
    

    return motifs_cnts;
}

vector<long long int> GraphSearch::sixNode112SampleAndCheckMotif(
    long long int max_trial,
    discrete_distribution<>& e2_weights_distr,
    vector<long long int>& e1_sampling_weights,
    vector<long long int>& e3_sampling_weights
    )
{
    random_device rd;
    mt19937 eng(rd() ^ omp_get_thread_num());

    vector<long long int> motifs_cnts(2, 0);

    for(long long int trial=0; trial < max_trial; trial++)
    {
        vector<Edge> sampled_edges(5, Edge(-1,-1,-1,-1));
        // int edge_id = e2_weight_distr(eng);
        bool found = sample112(
            trial, eng, e2_weights_distr, e1_sampling_weights, e3_sampling_weights, sampled_edges);
        if(!found)
        {
            continue;
        }
        vector<long long int> this_motifs_cnts = check_motif112(sampled_edges);
        for(int i = 0; i < motifs_cnts.size(); i++)
        {
            motifs_cnts[i] += this_motifs_cnts[i];
        }
    }

    return motifs_cnts;
}


long long int GraphSearch::sixNode112PreprocessSamplingWeights(
    vector<long long int>& e1_sampling_weights,
    vector<long long int>& e3_sampling_weights,
    vector<long long int>& e2_sampling_weights
    )
{
    /*
    e1_sampling_weights, e3_sampling_wegiths ensure that the timestamp
    e0 <= e1 <= e0+delta; e3 <= e4 <= e3+delta
    e2_sampling_weights ensure that the timesamp
    e1 <= e2 <= e1+delta; e2 <= e3 <= e2+delta
    */
    long long int W = 0;

    #pragma omp parallel for reduction(+:W)
    for(int i = 0; i < _g->numEdges(); i++)
    {
        Edge e = _g->edges()[i];
        int e_src = e.source();
        int e_dst = e.dest();
        if(e_src == e_dst){
            continue;
        }
        const vector<int>& e_src_in_edges = _g->nodes()[e_src].inEdges();
        const vector<int>& e_dst_out_edges = _g->nodes()[e_dst].outEdges();
        // e_src_in_edges.index() < e.index()
        vector<int>::const_iterator e_src_in_edges_right_it = lower_bound(
            e_src_in_edges.begin(), e_src_in_edges.end(), e.index()
        );
        // e_src_in_edges.time() >= e.time() - _delta
        vector<int>::const_iterator e_src_in_edges_left_it = lower_bound(
            e_src_in_edges.begin(), e_src_in_edges.end(), e.time() - _delta,
            [&](const int &a, const time_t &b) { return _g->edges()[a].time() < b; }
        );
        int num_e_src_in_edges = distance(e_src_in_edges_left_it, e_src_in_edges_right_it);
        e1_sampling_weights[i] = (long long int) num_e_src_in_edges;
        
        // e_dst_out_edges.index() > e.index()
        vector<int>::const_iterator e_dst_out_edges_left_it = upper_bound(
            e_dst_out_edges.begin(), e_dst_out_edges.end(), e.index()
        );
        // e_dst_out_edges.time() <= e.time() + _delta
        vector<int>::const_iterator e_dst_out_edges_right_it = upper_bound(
            e_dst_out_edges.begin(), e_dst_out_edges.end(), e.time() + _delta,
            [&](const time_t &a, const int &b) { return a < _g->edges()[b].time(); }
        );
        int num_e_dst_out_edges = distance(e_dst_out_edges_left_it, e_dst_out_edges_right_it);
        e3_sampling_weights[i] =  (long long int) num_e_dst_out_edges;
    }

    #pragma omp parallel for reduction(+:W)
    for(int i = 0; i < _g->numEdges(); i++)
    {
        Edge e2 = _g->edges()[i];
        int u = e2.source();
        int v = e2.dest();
        if (u == v)
            e2_sampling_weights[i] = 0;
        else
        {
            const vector<int>& e_src_in_edges = _g->nodes()[u].inEdges();
            const vector<int>& e_dst_out_edges = _g->nodes()[v].outEdges();
            // e_src_in_edges.index() < e2.index()
            vector<int>::const_iterator e_src_in_edges_right_it = lower_bound(
                e_src_in_edges.begin(), e_src_in_edges.end(), e2.index()
            );
            // e_src_in_edges.time() >= e2.time() - _delta
            vector<int>::const_iterator e_src_in_edges_left_it = lower_bound(
                e_src_in_edges.begin(), e_src_in_edges.end(), e2.time() - _delta,
                [&](const int &a, const time_t &b) { return _g->edges()[a].time() < b; }
            );
            long long int e2_u_weight = 0;
            for(vector<int>::const_iterator it = e_src_in_edges_left_it; it != e_src_in_edges_right_it; it++)
            {
                e2_u_weight += e1_sampling_weights[*it];
            }

            // e_dst_out_edges.index() > e2.index()
            vector<int>::const_iterator e_dst_out_edges_left_it = upper_bound(
                e_dst_out_edges.begin(), e_dst_out_edges.end(), e2.index()
            );
            // e_dst_out_edges.time() <= e2.time() + _delta
            vector<int>::const_iterator e_dst_out_edges_right_it = upper_bound(
                e_dst_out_edges.begin(), e_dst_out_edges.end(), e2.time() + _delta,
                [&](const time_t &a, const int &b) { return a < _g->edges()[b].time(); }
            );

            long long int e2_v_weight = 0;
            for(vector<int>::const_iterator it = e_dst_out_edges_left_it; it != e_dst_out_edges_right_it; it++)
            {
                e2_v_weight += e3_sampling_weights[*it];
            }
            e2_sampling_weights[i] = e2_u_weight * e2_v_weight;
            W += e2_sampling_weights[i];
        }
    }
    return W;
}

vector<float> GraphSearch::estimate_motif_general(const vector<long long int> &motifs_cnts, int num_sample, long long int W)
{
    vector<float> estimated_cnts(motifs_cnts.size(), 0);
    float_t W_div_k = (float_t) W / num_sample;
    for(size_t i=0; i < motifs_cnts.size(); i++)
    {
        estimated_cnts[i] = (float) motifs_cnts[i]  * W_div_k;
    }
    return estimated_cnts;
}

vector<float> GraphSearch::sixNode112PathSample(const Graph &g,
                                                int num_of_threads, int partition_per_thread,
                                                int delta, long long int max_trial)
{
    // Store class data structures
    _g = &g;
    _delta = delta;

    bool debugOutput = false;

    // Stores the matching subgraphs as list of edge indices
    long long int results = 0;
    // long long int results = 0;

    int n = _g->numNodes();
    int m = _g->numEdges();

    long long int success = 0;
    // long long int trial = 0;
    long long int W = 0;

    // motifs counts
    vector<long long int> motifs_cnts(2, 0);

    vector<long long int> e1_sampling_weights(_g->numEdges(), 0);
    vector<long long int> e2_sampling_weights(_g->numEdges(), 0);
    vector<long long int> e3_sampling_weights(_g->numEdges(), 0);

    W = sixNode112PreprocessSamplingWeights(
        e1_sampling_weights, e3_sampling_weights, e2_sampling_weights);

    cout << "W: " << W << endl;
    // cout << "e2_sampling_weights: ";
    // for(int i = 0; i < e2_sampling_weights.size(); i++)
    // {
    //     cout << e2_sampling_weights[i] << ", ";
    // }
    // cout << endl;
    discrete_distribution<> e2_weights_distr(e2_sampling_weights.begin(), e2_sampling_weights.end());

    motifs_cnts = sixNode112SampleAndCheckMotif(
        max_trial, e2_weights_distr, e1_sampling_weights, e3_sampling_weights);

    cout << "motifs_cnts: " << motifs_cnts[0] << endl;

    vector<float> estimted_cnts = estimate_motif_general(motifs_cnts, max_trial, W);

    return estimted_cnts;
}
