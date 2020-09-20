select T.hometown, avg(C.level)
from Trainer T, CatchedPokemon C
where T.id = C.owner_id
group by T.hometown
order by avg(C.level);
