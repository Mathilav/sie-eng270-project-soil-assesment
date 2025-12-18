function plot_poro(input_file_V, input_file_P, input_file_T, output_png_file)

%% Lecture des deux fichiers de BD
V = readtable(input_file_V);

% Conversion des dates AAAAMMJJ -> datetime
V.Date_interv = datetime(V.Date_interv, ...
                         'ConvertFrom','yyyymmdd');   % ex: 19910305 -> 05/03/1991

% ---- Bulk_density2_modif2.csv  -> P ----
P = readtable(input_file_P);
P.lower_limit_int = ceil(P.lower_limit_int / 5) * 5;   % arrondi multiple de 5

% ---- Bulk_density1_modif.csv  -> T ----
T = readtable(input_file_T);

%% Concaténation verticale : T puis P
Tsub = T(:, {'Plot','DateOfMeasurement','LowerLimit_cm_','Poro_mean'});
Tsub.Properties.VariableNames = {'Plot','DateOfMeasurement','lower_limit_int','Porosity'};

Psub = P(:, {'Plot','DateOfMeasurement','lower_limit_int','Poro'});
Psub.Properties.VariableNames = {'Plot','DateOfMeasurement','lower_limit_int','Porosity'};

BigTable = [Tsub ; Psub];

%% Filtre horizon 1 et plot pour BigTable
horizon1 = 5;
plot1    = 1;

mask_h1_Big = (BigTable.lower_limit_int == horizon1);
Big_h1      = BigTable(mask_h1_Big, :);

mask_plot1  = (Big_h1.Plot == plot1);
big_final1  = Big_h1(mask_plot1,:);

dates_Big_h1 = big_final1.DateOfMeasurement;
poro_Big_h1  = big_final1.Porosity;

%% Filtre correspondant dans V pour horizon 1
h1 = 1;
mask_h1_V   = (V.Horizon == h1);
filtered_V_h1 = V(mask_h1_V, :);

dates_V_h1 = filtered_V_h1.Date_interv;
poro_V_h1  = filtered_V_h1.Porosity;

%% Filtre horizon 5 (25 cm dans BigTable) et plot pour BigTable
horizon5 = 25;
plot1    = 1;

mask_h5_Big = (BigTable.lower_limit_int == horizon5);
Big_h5      = BigTable(mask_h5_Big, :);

mask_plot1_5 = (Big_h5.Plot == plot1);
big_final5   = Big_h5(mask_plot1_5,:);

dates_Big_h5 = big_final5.DateOfMeasurement;
poro_Big_h5  = big_final5.Porosity;

%% Filtre correspondant dans V pour horizon 5
h5 = 5;
mask_h5_V    = (V.Horizon == h5);
filtered_V_h5 = V(mask_h5_V, :);

dates_V_h5 = filtered_V_h5.Date_interv;
poro_V_h5  = filtered_V_h5.Porosity;

%% Calcul somme des pressions par jour
G = groupsummary(V, "Date_interv", "day", "sum", "PressionMax_kPa");
% G.day_Date_interv : dates (une par jour)
% G.sum_PressionMax_kPa : somme des pressions pour cette date

%% Plots : trois sous-graphes sur la même figure
figure;

% -------- Horizon 1 --------
subplot(3,1,1);
plot(dates_Big_h1, poro_Big_h1, '-o', 'DisplayName','BD (T+P) - H1');
hold on;
plot(dates_V_h1,   poro_V_h1,   '-s', 'DisplayName','Vérif - H1');
hold off;
xlabel('Date');
ylabel('Porosité');
title('Porosité vs temps, horizon 1 / plot 1');
legend('Location','best');
grid on;

% -------- Horizon 5 --------
subplot(3,1,2);
plot(dates_Big_h5, poro_Big_h5, '-o', 'DisplayName','BD (T+P) - H5');
hold on;
plot(dates_V_h5,   poro_V_h5,   '-s', 'DisplayName','Vérif - H5');
hold off;
xlabel('Date');
ylabel('Porosité');
title('Porosité vs temps, horizon 5 / plot 1');
legend('Location','best');
grid on;

% -------- Somme des pressions --------
subplot(3,1,3);
plot(dates_V_h5, G.sum_PressionMax_kPa, '-o', 'LineWidth', 1.5);
grid on;
xlabel('Date');
ylabel('Somme des pressions (kPa)');
title('Somme des pressions appliquées au sol par date');


% Save figure
saveas(gcf, output_png_file);

end