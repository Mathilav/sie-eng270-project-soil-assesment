%% Lecture des deux fichiers de BD
thisDir  = fileparts(mfilename('fullpath'));           
dataDir  = fullfile(thisDir, '..', 'data_input');      
resultDir = fullfile(thisDir,'..', 'results');

% ---- porosity_results -> V ----
csvPath3 = fullfile(resultDir, 'porosity_results.csv'); 
V = readtable(csvPath3);       % pas "data_BD_verif.m" !

% Conversion des dates AAAAMMJJ -> datetime
% (accepte numeric ou string/char)
V.Date_interv = datetime(V.Date_interv, ...
                         'ConvertFrom','yyyymmdd');      % ex: 19910305 -> 05/03/1991

% ---- Bulk_density2_modif1.csv  -> P ----
csvPath2 = fullfile(dataDir, 'Bulk_density2_modif1.csv');
P        = readtable(csvPath2);
P.LowerLimitInt = ceil(P.LowerLimitInt / 5) * 5;       

% ---- Bulk_density1_modif.csv  -> T ----
csvPath1 = fullfile(dataDir, 'Bulk_density1_modif.csv');
T        = readtable(csvPath1);

%% Concaténation verticale : T puis P
Tsub = T(:, {'Plot','DateOfMeasurement','LowerLimit_cm_','Poro_mean'});
Tsub.Properties.VariableNames = {'Plot','DateOfMeasurement','LowerLimit','Porosity'};

Psub = P(:, {'Plot','DateOfMeasurement','LowerLimitInt','Poro'});
Psub.Properties.VariableNames = {'Plot','DateOfMeasurement','LowerLimit','Porosity'};

BigTable = [Tsub ; Psub];

%% Filtre horizon et plot pour BigTable
horizon1 = 5;
plot1    = 1;

mask_h1_Big = (BigTable.LowerLimit == horizon1);
Big_h1      = BigTable(mask_h1_Big, :);       % (corrige mask_h1_T -> mask_h1_Big)

mask_plot1  = (Big_h1.Plot == plot1);
big_final   = Big_h1(mask_plot1,:);

dates_Big = big_final.DateOfMeasurement;      % Date_interv n'existe pas dans BigTable
poro_Big  = big_final.Porosity;

% Moyenne de la porosité par jour
DailyPorosity = groupsummary(big_final, ...
                             "DateOfMeasurement", ...   % variable de date
                             "day", ...                 % regrouper par jour calendaire
                             "mean", ...                % méthode
                             "Porosity");               % variable à moyenner


%% Filtre correspondant dans V
h1 = 1; 
mask_h1_V = (V.Horizon == h1); 
filtered_V = V(mask_h1_V, :);

dates_V = filtered_V.Date_interv;             % en supposant cette colonne dans V
poro_V  = filtered_V.Porosity;

%% Plot sur le même graphique
figure;
plot(dates_Big, poro_Big, '-o', 'DisplayName','BD (T+P)');
hold on;
plot(dates_V,   poro_V,   '-s', 'DisplayName','Vérif');
hold off;

xlabel('Date');
ylabel('Porosité');
title('Porosité vs temps, horizon 1 / plot 1');
legend('Location','best');
grid on;
