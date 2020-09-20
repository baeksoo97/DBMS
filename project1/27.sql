select T.name, max(C.level)
from Trainer T, CatchedPokemon C
where T.id = C.owner_id
group by T.id
having count(T.id) >= 4
order by T.name;
