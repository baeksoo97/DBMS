select sum(C.level)
from Trainer T, CatchedPokemon C
where T.id = C.owner_id
and T.name = "Matis";

