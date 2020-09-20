select P.name, P.id
from Pokemon P, CatchedPokemon C, Trainer T
where P.id = C.pid
and T.id = C.owner_id
and T.hometown = "Sangnok City"
order by P.id;
