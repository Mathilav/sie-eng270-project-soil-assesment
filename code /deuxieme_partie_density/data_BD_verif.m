% Lire les deux fichiers
T = readtable("data_2/Bulk_density1.csv");
P = readtable("data_2/Bulk_density2.csv");

% Colonnes à garder (adapter si nécessaire aux VRAIS noms affichés ci-dessus)
vars_T = {'Plot', 'Date_interv', 'Crop', 'DateOfMeasurement', 'Poro_mean'};
vars_P = {'Plot', 'Date_interv', 'Crop', 'DateOfMeasurement', 'Poro'};

% Sous-tables avec uniquement les colonnes d'intérêt
first_table = T(:, vars_T);
sec_table   = P(:, vars_P);

% Harmoniser le nom de la variable de densité
sec_table = renamevars(sec_table, 'Poro', 'Poro_mean');

% Fusionner en une seule grande table
BD_all = [first_table; sec_table];   % concaténation verticale

% Exemple : partir d'une table BD_all déjà lue (ou T, ou P)
% Colonnes utilisées
nomColParcelle = 'Plot';
nomColDate     = 'DateOfMeasurement';

% 1) Trouver les couples (Parcelle, Date) uniques
[grpID, combinaisons] = findgroups(BD_all(:, {nomColParcelle, nomColDate}));

% 2) Compter le nombre de dates DISTINCTES par parcelle
%   -> on groupe uniquement par la parcelle
[grpParcelle, listeParcelles] = findgroups(BD_all.(nomColParcelle));
nbDatesParParcelle = splitapply(@(d) numel(unique(d)), BD_all.(nomColDate), grpParcelle);

% 3) Mettre le résultat dans une table récapitulative
Resume = table(listeParcelles, nbDatesParParcelle, ...
               'VariableNames', {'Parcelle','NbDatesDistinctes'});

% 4) Trouver la parcelle avec le plus grand nombre de dates distinctes
[nbMax, idxBest] = max(Resume.NbDatesDistinctes);
meilleureParcelle = Resume.Parcelle(idxBest);

% 2) Afficher la meilleure parcelle en texte
disp('Parcelle avec le plus de dates distinctes :')
disp(meilleureParcelle)

disp('Nombre de dates distinctes pour cette parcelle :')
disp(nbMax)

% Créer le tableau avec toutes les data de la meilleure parcelle
mask_best = BD_all.(nomColParcelle) == meilleureParcelle;
Table_meilleureParcelle = BD_all(mask_best, :);

% Afficher un résumé rapide
disp('Table de la parcelle sélectionnée :')
summary(Table_meilleureParcelle)

writetable( Table_meilleureParcelle, 'Data_bulk_density.csv'); 
