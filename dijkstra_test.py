import dijkstra

A,B,C,D,E,F = nodes = list("ABCDEF")

graph = dijkstra.Graph()

graph.add_edge(A,B,1)
graph.add_edge(B,C,1)
graph.add_edge(C,B,1)
graph.add_edge(D,A,1)
graph.add_edge(D,B,1)
graph.add_edge(E,D,1)
graph.add_edge(F,D,1)
graph.add_edge(F,E,1)

dijkstraC = dijkstra.DijkstraSPF(graph, C) #the shortest distance to each node, starting from F, is available
dijkstraD = dijkstra.DijkstraSPF(graph, D) #the shortest distance to each node, starting from F, is available
dijkstraE= dijkstra.DijkstraSPF(graph, E) #the shortest distance to each node, starting from F, is available
dijkstraF = dijkstra.DijkstraSPF(graph, F) #the shortest distance to each node, starting from F, is available

print(" -> ".join(dijkstraC.get_path(B))) #we can extract the path. From C to B, the path is:
print(" -> ".join(dijkstraD.get_path(B))) #we can extract the path. From D to B, the path is:
print(" -> ".join(dijkstraE.get_path(B))) #we can extract the path. From E to B, the path is:
print(" -> ".join(dijkstraF.get_path(B))) #we can extract the path. From F to B, the path is: