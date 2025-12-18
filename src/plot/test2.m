%% Filtre horizon 1 et plot pour BigTable
horizon1 = 5;
plot1    = 1;

mask_h1_Big = (BigTable.LowerLimit == horizon1);
Big_h1      = BigTable(mask_h1_Big, :);          % lignes horizon 1
mask_plot1  = (Big_h1.Plot == plot1);
big_final1  = Big_h1(mask_plot1,:);

dates_Big_h1 = big_final1.DateOfMeasurement;     % Date_interv n'existe pas dans BigTable
poro_Big_h1  = big_final1.Porosity;

%% Filtre correspondant dans V pour horizon 1
h1 = 1; 
mask_h1_V = (V.Horizon == h1); 
filtered_V_h1 = V(mask_h1_V, :);

dates_V_h1 = filtered_V_h1.Date_interv;          % en supposant cette colonne dans V
poro_V_h1  = filtered_V_h1.Porosity;

%% Filtre horizon 5 (25 dans BigTable) et plot pour BigTable
horizon5 = 25;
plot1    = 1;                                    % même plot

mask_h5_Big = (BigTable.LowerLimit == horizon5);
Big_h5      = BigTable(mask_h5_Big, :);          % lignes horizon 5
mask_plot1_5 = (Big_h5.Plot == plot1);
big_final5   = Big_h5(mask_plot1_5,:);

dates_Big_h5 = big_final5.DateOfMeasurement;
poro_Big_h5  = big_final5.Porosity;

%% Filtre correspondant dans V pour horizon 5
h5 = 5; 
mask_h5_V = (V.Horizon == h5); 
filtered_V_h5 = V(mask_h5_V, :);

dates_V_h5 = filtered_V_h5.Date_interv;
poro_V_h5  = filtered_V_h5.Porosity;

%% Plots : deux sous-graphes sur la même figure

figure;

% -------- Horizon 1 --------
subplot(2,1,1);
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
subplot(2,1,2);
plot(dates_Big_h5, poro_Big_h5, '-o', 'DisplayName','BD (T+P) - H5');
hold on;
plot(dates_V_h5,   poro_V_h5,   '-s', 'DisplayName','Vérif - H5');
hold off;
xlabel('Date');
ylabel('Porosité');
title('Porosité vs temps, horizon 5 / plot 1');
legend('Location','best');
grid on;
