function plot_poro(input_file_V, input_file_P, input_file_T, output_png_file)
    % plot_poro
    %   Plot measured and modeled porosity and the daily sum of pressures
    %   for two soil horizons (5 cm and 25 cm) and plot 1.
    %
    % Parameters
    %   input_file_V    : path to verification porosity file (V)
    %   input_file_P    : path to bulk density file part 2 (P)
    %   input_file_T    : path to bulk density file part 1 (T)
    %   output_png_file : path to output PNG file
    
        %% Read input tables
        V = readtable(input_file_V);
        P = readtable(input_file_P);
        T = readtable(input_file_T);
    
        % Convert numeric dates (YYYYMMDD) to datetime
        V.Date_interv = datetime(V.Date_interv, 'ConvertFrom', 'yyyymmdd');
    
        % Round lower depth limits in P to the next multiple of 5 cm
        P.lower_limit_int = ceil(P.lower_limit_int / 5) * 5;
    
        %% Build combined bulk density table (T followed by P)
        Tsub = T(:, {'Plot', 'DateOfMeasurement', 'LowerLimit_cm_', 'Poro_mean'});
        Tsub.Properties.VariableNames = {'Plot', 'DateOfMeasurement', ...
                                         'lower_limit_int', 'Porosity'};
    
        Psub = P(:, {'Plot', 'DateOfMeasurement', 'lower_limit_int', 'Poro'});
        Psub.Properties.VariableNames = {'Plot', 'DateOfMeasurement', ...
                                         'lower_limit_int', 'Porosity'};
    
        BigTable = [Tsub ; Psub];
    
        %% Select horizon 1 (5 cm) for plot 1 from BigTable
        horizon1_depth_cm = 5;
        target_plot       = 1;
    
        mask_h1_Big = (BigTable.lower_limit_int == horizon1_depth_cm);
        Big_h1      = BigTable(mask_h1_Big, :);
    
        mask_plot1  = (Big_h1.Plot == target_plot);
        big_final1  = Big_h1(mask_plot1, :);
    
        dates_big_h1 = big_final1.DateOfMeasurement;
        poro_big_h1  = big_final1.Porosity;
    
        %% Select corresponding records in V for horizon index 1
        horizon_index_h1 = 1;
    
        mask_h1_V     = (V.Horizon == horizon_index_h1);
        filtered_V_h1 = V(mask_h1_V, :);
    
        dates_V_h1 = filtered_V_h1.Date_interv;
        poro_V_h1  = filtered_V_h1.Porosity;
    
        %% Select horizon 5 (25 cm) for plot 1 from BigTable
        horizon5_depth_cm = 25;
    
        mask_h5_Big = (BigTable.lower_limit_int == horizon5_depth_cm);
        Big_h5      = BigTable(mask_h5_Big, :);
    
        mask_plot1_5 = (Big_h5.Plot == target_plot);
        big_final5   = Big_h5(mask_plot1_5, :);
    
        dates_big_h5 = big_final5.DateOfMeasurement;
        poro_big_h5  = big_final5.Porosity;
    
        %% Select corresponding records in V for horizon index 5
        horizon_index_h5 = 5;
    
        mask_h5_V     = (V.Horizon == horizon_index_h5);
        filtered_V_h5 = V(mask_h5_V, :);
    
        dates_V_h5 = filtered_V_h5.Date_interv;
        poro_V_h5  = filtered_V_h5.Porosity;
    
        %% Compute daily sum of maximum pressure
        pressure_daily = groupsummary(V, "Date_interv", "day", "sum", "PressionMax_kPa");
        % pressure_daily.day_Date_interv      : dates (one per day)
        % pressure_daily.sum_PressionMax_kPa : daily sum of pressures
    
        %% Plots: three subplots in a single figure
        fig = figure('Visible', 'on');
    
        % -------- Horizon 1 --------
        
        subplot(3, 1, 1);
        plot(dates_big_h1, poro_big_h1, '-o', 'DisplayName', 'Mesures H1');
        hold on;
        plot(dates_V_h1,   poro_V_h1,   '-s', 'DisplayName', 'Porosity modele - H1');
        hold off;
        xlabel('Date');
        ylabel('Porosity');
        title('Measured and Modeled Porosity Over Time – Horizon 1');
        legend('Location', 'best');
        grid on;
    
        % -------- Horizon 5 --------
        subplot(3, 1, 2);
        plot(dates_big_h5, poro_big_h5, '-o', 'DisplayName', 'Mesures H5');
        hold on;
        plot(dates_V_h5,   poro_V_h5,   '-s', 'DisplayName', 'Porosity modele - H5');
        hold off;
        xlabel('Date');
        ylabel('Porosity');
        title('Measured and Modeled Porosity Over Time – Horizon 5');
        legend('Location', 'best');
        grid on;
    
        % -------- Sum of pressures --------
        subplot(3, 1, 3);
        plot(dates_V_h5, pressure_daily.sum_PressionMax_kPa, '-o', 'LineWidth', 1.5);
        grid on;
        xlabel('Date');
        ylabel('Sum of Pressures (kPa)');
        title('Total Applied Soil Pressure Over Time');
        % Save figure
        drawnow;
        exportgraphics(fig, output_png_file, 'Resolution', 300);
    end
    