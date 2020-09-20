select distinct T.name
from Trainer T, CatchedPokemon C
where T.id = C.owner_id
and C.level <= 10
order by T.name;
