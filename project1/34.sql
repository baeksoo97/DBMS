select P.name, C.level, C.nickname 
from Trainer T, CatchedPokemon C, Gym G, Pokemon P
where T.id = C.owner_id
and T.id = G.leader_id
and P.id = C.pid
and C.nickname like "A%"
order by P.name desc;
