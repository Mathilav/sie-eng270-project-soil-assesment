%% 1) Lecture des fichiers
T = readtable('deuxieme_partie_density/porosity_results.csv');
P = readtable('deuxieme_partie_density/data_2/Bulk_density1.csv');

%% 2) Dates de T : AAAAMMJJ -> datetime JJ/MM/AAAA
if isnumeric(T.Date_interv)
    T.Date_interv = datetime(T.Date_interv, 'ConvertFrom','yyyymmdd');
else
    T.Date_interv = datetime(string(T.Date_interv), 'InputFormat','yyyyMMdd');
end
T.Date_interv.Format = 'dd/MM/yyyy';

%% 3) Correction des dates dans P (2 chiffres d'année -> 19xx), UNE FOIS
date_str_short = string(datestr(P.DateOfMeasurement, 'dd/mm/yy'));  % "12/11/92"
parts    = split(date_str_short, "/");
dayStr   = parts(:,1);
monthStr = parts(:,2);
year2Str = parts(:,3);
year4Str = "19" + year2Str;                       % "92" -> "1992"
date_str_full = dayStr + "/" + monthStr + "/" + year4Str;
P.DateOfMeasurement = datetime(date_str_full, 'InputFormat','dd/MM/yyyy');
P.DateOfMeasurement.Format = 'dd/MM/yyyy';

%% 4) Filtrage des données
horizon1 = 1;
plot1    = 1;

% P : Horizon = 1 & Plot = 1
mask_h1_P = (P.Horizon == horizon1);
mask_p1   = (P.Plot    == plot1);
P_sel     = P(mask_h1_P & mask_p1, :);

% S'assurer que Poro_mean est numérique
if isnumeric(P_sel.Poro_mean)
    poro_P = P_sel.Poro_mean;
else
    tmp    = strtrim(string(P_sel.Poro_mean));
    tmp    = strrep(tmp, ',', '.');
    poro_P = str2double(tmp);
    P_sel.Poro_mean = poro_P;
end

%% 5) Moyenne par jour de Poro_mean dans P_sel, SANS toucher aux dates
dates_P = P_sel.DateOfMeasurement;   % datetime déjà corrigé

% Dates uniques (jours)
uniqueDates = unique(dates_P);

% Pré-allocation
poroP_mean  = nan(size(uniqueDates));
for k = 1:numel(uniqueDates)
    d = uniqueDates(k);
    % logique : même jour (ignorer l'heure si jamais il y en avait)
    mask_d = dateshift(dates_P,'start','day') == dateshift(d,'start','day');
    poroP_mean(k) = mean(poro_P(mask_d), 'omitnan');
end
datesP_mean = uniqueDates;   % mêmes datetime que dans P_sel

%% 6) Plot : seulement la moyenne quotidienne de P (en rouge, très visible)
figure;
hold on;

% Points moyens par jour (rouge, taille 80)
scatter(datesP_mean, poroP_mean, 80, 'r', 'filled', ...
        'DisplayName','Poro\_mean quotidien (P)');

hold off;
xlabel('Date (JJ/MM/AAAA)');
ylabel('Porosité moyenne (P)');
title('Moyenne quotidienne de Poro\_mean (Horizon 1, Plot 1)');
legend('Location','best');
grid on;

ax = gca;
ax.XAxis.TickLabelFormat = 'dd/MM/yyyy';
