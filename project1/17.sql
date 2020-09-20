select count(distinct C.pid)
from CatchedPokemon C, Trainer T
where C.owner_id = T.id
and T.hometown = "Sangnok City";
