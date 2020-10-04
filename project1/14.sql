select P.name
from Pokemon P, Evolution E
where P.id = E.before_id
and P.type = "Grass";

