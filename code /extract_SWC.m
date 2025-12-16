% extraction des data pour soil water content 
T = readtable('real_SWC.csv'); 

num_parcelle = 1;

mask_parcelle1 = (T.Plot == num_parcelle);
lignes_parcelle1 = T(mask_parcelle1, :);
var = {'Date_SWC', 'Horizon', 'SWC_mean'}; 
table_operation = lignes_parcelle1(:, var); 


% 1) Lire le fichier de porosité par horizon
P = readtable('../deuxieme_partie_density/porosity_results.csv');

% 2) Moyenne journalière de Porosity (une ligne par date)
Pmean = groupsummary(P, "Date_interv", "mean", "Porosity");
Pmean.Properties.VariableNames{'mean_Porosity'} = 'Porosity_mean';

% 3) Ne garder que la date et la moyenne
Pmean = Pmean(:, {'Date_interv','Porosity_mean'});   % seulement clé + moyenne

% 4) Sauvegarder
writetable(Pmean, "porosity_mean_per_day.csv");