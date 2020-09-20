select count(P.type)
from CatchedPokemon C, Pokemon P
where C.pid = P.id
group by P.type
order by P.type;


