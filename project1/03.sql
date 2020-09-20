select avg(level)
from CatchedPokemon as C, Trainer as T, Pokemon as P
where C.owner_id = T.id
and T.hometown = "Sangnok City"
and C.pid = P.id
and P.type = "Electric";
