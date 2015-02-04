:-lib(timeout).
test_algo_list(Cpu,Memory, Output) :-
         findall(X ,(numanode(X,Y,Z),Y>Cpu,Z>Memory),Output).

list_all(_, _, Output) :-
        findall((X,Y,Z), numanode(X,Y,Z), Output).

list_all_telephone(Output) :-
        findall((X), telephone(X), Output).

test_timeout(X) :-
	timeout((repeat,fail), X, writeln(time-out)).


%list_all_numa_nodes_info(Cpu,Memory,Output) :-
 %      findall((X,Y,Z,A,B,C), nodeinfo(X,Y,Z,A,B,C),Output).

get_free_numa_node(Cpu,Memory,Output) :-
        findall(X, (nodeinfo(X,_,_,A,_,C), A>Memory,C>Cpu),Output).

get_meminfo_list(Output) :-
         findall((X,Y) ,meminfo(X,Y),Output).

delete_numa_node_info(Node, _, _) :-
        retract(nodeinfo(Node,_,_,_,_,_)).

get_cpuinfo_list(Output) :-
         findall((X,Y) ,cpuinfo(X,Y),Output).
