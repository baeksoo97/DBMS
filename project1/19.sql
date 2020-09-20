select count(distinct P.type)
from CatchedPokemon C, Trainer T, Gym G, Pokemon P
where C.owner_id = T.id
and G.leader_id = T.id
and C.pid= P.id
and G.city = "Sangnok City";
