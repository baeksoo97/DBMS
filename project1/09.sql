select T.name, avg(C.level)
from Trainer T, CatchedPokemon C, Gym G
where T.id = C.owner_id
and T.id = G.leader_id
group by T.id
order by T.name;
