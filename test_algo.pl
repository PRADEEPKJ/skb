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
       %findall((X,Y,Z,A,B,C), nodeinfo(X,Y,Z,A,B,C,I),Output).

list_node_info(Output) :-
       findall((X,Y,Z,A,B,C,I), nodeinfo(X,Y,Z,A,B,C,I),Output).


get_free_numa_node(Memory,Cpu,Output) :-
        findall(X, (nodeinfo(X,_,_,A,_,C,_), A>Memory,C>Cpu),Output).

get_node_sysinfo(Output) :-
        findall((Y,Z,A,B), sysinfo(Y,Z,A,B),Output).


get_all_sysinfo(Type,Memory,Cpu,Output) :-
        findall(Y, (sysinfo(T,M,C,Y),T==Type,C>Cpu,M>Memory),Output).


get_meminfo_list(Output) :-
         findall((X,Y) ,meminfo(X,Y),Output).

delete_numa_node_info(Node) :-
        retract(nodeinfo(Node,_,_,_,_,_,_)).

delete_sysinfo(Node) :-
        retract(sysinfo(Node,_,_,_)).


get_cpuinfo_list(Output) :-
         findall((X,Y) ,cpuinfo(X,Y),Output).
