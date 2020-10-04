select P.name
from Pokemon P, CatchedPokemon C
where P.id = C.pid
and C.nickname like "% %"
order by P.name desc;
