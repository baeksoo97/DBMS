select avg(level)
from CatchedPokemon C, Trainer T
where C.owner_id = T.id
and T.name = "Red";
