select P.name
from Pokemon as P, Evolution as E
where P.id = E.before_id
and E.after_id < E.before_id
order by P.name;
