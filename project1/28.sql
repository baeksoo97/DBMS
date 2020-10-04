select T.name, avg(C.level)
from CatchedPokemon C, Pokemon P, Trainer T
where C.pid = P.id
and C.owner_id = T.id
and P.type in ("Normal", "Electric")
group by T.id
order by avg(C.level);


