% NE FONCTIONNE PAAAAAAAAAS !!!!!!!!!!!!!


%% 1) Lecture des fichiers
T  = readtable('deuxieme_partie_density/data_2/Bulk_density1.csv');
P  = readtable('deuxieme_partie_density/data_2/Bulk_density2.csv');
Ta = readtable('deuxieme_partie_density/porosity_results.csv');

%% 2) Corriger la colonne LowerLimit dans P (virgule -> point -> double)

% Cas le plus fréquent : LowerLimit est un cell array de chaînes du type '12,5'
if iscell(P.LowerLimit)
    % Nettoyage texte
    raw = P.LowerLimit;                  % cell de char
    raw = strtrim(raw);                  % enlever espaces
    raw = strrep(raw, ',', '.');         % remplacer virgule par point

    % Conversion en double
    P.LowerLimit = str2double(raw);      % vecteur double (NaN si non numérique)
elseif isstring(P.LowerLimit)
    raw = strtrim(P.LowerLimit);
    raw = strrep(raw, ',', '.');
    P.LowerLimit = str2double(raw);
end
% À ce stade, P.LowerLimit doit être numérique proprement importé.[web:132][web:225]

%% 3) Filtre sur la partie compacte

% Dans T : Partit textuel (cell de char), filtrer 'L1' par exemple
mask_partie_L1 = strcmp(T.Partit, 'L1');    % changer 'L1' en 'L2' si besoin
lignes_file1   = T(mask_partie_L1, :);

% Dans P : Partit déjà numérique (double)
partie_compacte_1 = 1;
mak_parti_1       = (P.Partit == partie_compacte_1);
lignes_file2      = P(mak_parti_1, :);

%% 4) Sélection des colonnes d'intérêt

vars_T = {'Plot', 'DateOfMeasurement', 'LowerLimit_cm_', 'Poro_mean'};
vars_P = {'Plot', 'DateOfMeasurement', 'LowerLimit',     'Poro'};

first_table = lignes_file1(:, vars_T);
sec_table   = lignes_file2(:, vars_P);

%% 5) Harmoniser les noms de colonnes avant concaténation

% Renommer Poro en Poro_mean
sec_table = renamevars(sec_table, 'Poro', 'Poro_mean');

% Forcer les mêmes noms que dans first_table
sec_table.Properties.VariableNames = first_table.Properties.VariableNames;

%% 6) Fusion

Poro_all = [first_table; sec_table];
writetable(Poro_all, 'Poro_all.csv')