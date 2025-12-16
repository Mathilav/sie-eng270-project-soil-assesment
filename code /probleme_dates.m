
P = readtable('deuxieme_partie_density/data_2/Bulk_density1.csv');

% 1) Revenir à une chaîne JJ/MM/AA à partir du datetime actuel
date_str_short = string(datestr(P.DateOfMeasurement, 'dd/mm/yy'));  % "12/11/92"

% 2) Découper en jour, mois, année sur 2 chiffres
parts = split(date_str_short, "/");   % tableau N×3 de strings
dayStr   = parts(:,1);
monthStr = parts(:,2);
year2Str = parts(:,3);

% 3) Construire l'année complète en 19xx
year4Str = "19" + year2Str;          % "92" -> "1992"

% 4) Reconstruire une chaîne JJ/MM/AAAA
date_str_full = dayStr + "/" + monthStr + "/" + year4Str;  % "12/11/1992"

% 5) Convertir en datetime JJ/MM/AAAA
dates_fixed = datetime(date_str_full, 'InputFormat','dd/MM/yyyy');
dates_fixed.Format = 'dd/MM/yyyy';

% 6) Remplacer la colonne dans P
P.DateOfMeasurement = dates_fixed;

% 7) Vérification
P(1:10, {'DateOfMeasurement'})
varfun(@class, P(:, {'DateOfMeasurement'}), 'OutputFormat','table')
