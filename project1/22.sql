select P.type, count(P.type)
from Pokemon P
group by P.type
order by count(P.type), P.type;
