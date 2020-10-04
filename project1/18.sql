select avg(level)
from CatchedPokemon C, Trainer T, Gym G
where C.owner_id = T.id
and T.id = G.leader_id;
