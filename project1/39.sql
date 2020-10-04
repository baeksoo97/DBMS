select T.name
from CatchedPokemon as C1, CatchedPokemon as C2, Trainer as T
where C1.id <> C2.id 
and C1.pid = C2.pid
and C1.owner_id = C2.owner_id
and T.id = C1.owner_id
group by T.id
order by T.name;
