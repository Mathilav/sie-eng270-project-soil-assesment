%% Load theta data (model output)
data = readtable('theta_output.csv');

% Convert YYYYMMDD to datetime
dates_model = datetime(string(data.Date), 'InputFormat', 'yyyyMMdd');
theta = data.Theta;

%% Load measured SWC data
T = readtable('real_SWC.csv');

num_parcelle = 1;
mask_parcelle1 = (T.Plot == num_parcelle);
lignes_parcelle1 = T(mask_parcelle1, :);

% Keep only relevant variables
table_operation = lignes_parcelle1(:, {'Date_SWC','SWC_mean'});


%% ---- FIX SWC DATES ----
% Date_SWC examples: 31/01/0094, 14/04/0002

dates_obs = datetime(table_operation.Date_SWC, ...
                     'InputFormat','dd/MM/yyyy');

% Fix wrong centuries
y = year(dates_obs);

% 0000–0029  → 2000–2029
dates_obs(y < 30) = dates_obs(y < 30) + calyears(2000);

% 0030–0099 → 1930–1999
dates_obs(y >= 30 & y < 100) = dates_obs(y >= 30 & y < 100) + calyears(1900);

%% ---- FIX SWC VALUES ----
% Convert from '21,4' → 21.4 → fraction
SWC_num = str2double(strrep(table_operation.SWC_mean, ',', '.'));
SWC_obs = SWC_num / 100;

%% % Compute average SWC on the root zone
% Dates uniques (jours)
uniqueDates = unique(dates_obs);

% Pré-allocation
SWC_rz  = nan(size(uniqueDates));
for k = 1:numel(uniqueDates)
    d = uniqueDates(k);
    % logique : même jour (ignorer l'heure si jamais il y en avait)
    mask_d = dateshift(dates_obs,'start','day') == dateshift(d,'start','day');
    SWC_rz(k) = mean(SWC_obs(mask_d), 'omitnan');
end
datesSWC_rz = uniqueDates;   % ce sont les mêmes datetime que dans P_sel, sans recalcule



%% Smooth theta (harmonized curve)
theta_smooth = smoothdata(theta, 'movmean', 15);

%% ---- PLOT ----
figure;

% Smoothed theta → BLUE
plot(dates_model, theta_smooth, ...
     'Color', [0 0.4470 0.7410], ... % MATLAB default blue
     'LineWidth', 2);
hold on;

% Raw theta → PURPLE
plot(dates_model, theta, '.', ...
     'Color', [0.4940 0.1840 0.5560], ... % purple
     'MarkerSize', 6);

% Observed SWC (real theta) → RED
plot(datesSWC_rz, SWC_rz, 'o', ...
     'MarkerSize', 6, ...
     'MarkerFaceColor', [0.8500 0.3250 0.0980], ... % red
     'MarkerEdgeColor', 'k');
hold off;

xlabel('Date');
ylabel('Soil water content \theta (cm^3 cm^{-3})');
title('Temporal evolution of root-zone soil water content');

legend('Smoothed \theta', 'Daily \theta', 'Observed \theta', ...
       'Location', 'best');

grid on;
set(gca, 'FontSize', 12);

% Save figure
saveas(gcf, 'theta_obsVScomp_plot.png');
