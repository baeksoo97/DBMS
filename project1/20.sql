select T.name, count(T.name)
from Trainer T, CatchedPokemon C
where T.id = C.owner_id
and T.hometown = "Sangnok City"
group by T.id
order by count(T.name);
