// GSW.cpp: 콘솔 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#include "time.h"
#include "chrono"
#include "iostream"
#include "stdlib.h"
#include "math.h"
#include "random"
#include "vector"
#include "fstream"
#include "string"

#include "windows.h"
#include "sysinfoapi.h"
long int beginTime;

using namespace std;

unsigned __int64 mod_q_ = pow(2, 48) - 1;
vector<double> errors = vector<double>();

std::random_device random;
std::mt19937_64 generator(random());
std::uniform_int_distribution<unsigned __int64> distribution;

void print(vector<vector<unsigned __int64>> m) {
	for (int i = 0;i < m.size();i++) {
		for (int j = 0;j < m[0].size();j++) {
			printf("%I64u\t", m[i][j]);
		}
		printf("\n");
	}
	printf("---------------\n");
}
void print(vector<vector<double>> m) {
	for (int i = 0;i < m.size();i++) {
		for (int j = 0;j < m[i].size();j++) {
			printf("%.10g\t", m[i][j]);
		}
		printf("\n");
	}
	printf("---------------\n");
}

// >>>>>> For GSW
vector<vector<unsigned __int64>> splitm(vector<vector<unsigned __int64>> c) {
	int q = 48;
	int nu = 16;
	int d = ceil(q / nu);
	unsigned __int64 nu_ = pow(2, nu) - 1;
	vector<vector<unsigned __int64>> result(c.size() * d, vector<unsigned __int64>(c[0].size(), 0));
	vector<vector<unsigned __int64>> c_temp(c.size(), vector<unsigned __int64>(c[0].size(), 0));
	for (int i = 0;i < c.size(); i++) {
		for (int j = 0;j < c[0].size();j++) {
			c_temp[i][j] = c[i][j];
		}
	}

	for (int piece = 0;piece < d;piece++) {
		for (int i = 0;i < c.size();i++) {
			for (int j = 0;j < c[0].size();j++) {
				result[piece * c.size() + i][j] = c_temp[i][j] & nu_;
				c_temp[i][j] = c_temp[i][j] >> (nu);
			}
		}
	}
	return result;
}
vector<vector<unsigned __int64>> mult(vector<vector<unsigned __int64>> encm, vector<vector<unsigned __int64>> split_enc) {
	vector<vector<unsigned __int64>> result(encm.size(), vector<unsigned __int64>(split_enc[0].size(), 0));

	int size_i = encm.size();
	int size_k = split_enc.size();
	int size_j = split_enc[0].size();

	for (int i = 0; i < size_i;i++) {
		for (int k = 0;k < size_k;k++) {
			for (int j = 0;j < size_j;j++) {
				result[i][j] = (result[i][j] + encm[i][k] * split_enc[k][j]) & mod_q_;
			}
		}
	}

	return result;
}
// <<<<<< For GSW

// >>>>>> For LWE
vector<vector<unsigned __int64>> scalar_mult(unsigned __int64 scalar, vector<vector<unsigned __int64>> c) {
	vector<vector<unsigned __int64>> result(c.size(), vector<unsigned __int64>(c[0].size(), 0));
	for (int i = 0;i < c.size();i++) {
		for (int j = 0;j < c[0].size();j++) {
			result[i][j] = (c[i][j] * scalar) & mod_q_;
		}
	}
	return result;
}
vector<vector<unsigned __int64>> add_enc(vector<vector<unsigned __int64>> c1, vector<vector<unsigned __int64>> c2) {
	vector<vector<unsigned __int64>> result(c1.size(), vector<unsigned __int64>(c1[0].size(), 0));
	for (int i = 0;i < c1.size();i++) {
		for (int j = 0;j < c1[0].size();j++) {
			result[i][j] = (c1[i][j] + c2[i][j]) & mod_q_;
		}
	}
	return result;
}
// <<<<<< For LWE

void startCount() {
	beginTime = GetTickCount();
}
void printInterval() {
	cout << GetTickCount() - beginTime << "\t[ms]"
		<< " ( " << (double)(GetTickCount() - beginTime) / 1000 << "\t[s] )" << endl;
}


class PIDControllerOrigin  {

private:
	vector<vector<double>> AB;
	vector<vector<double>> CD;
	vector<vector<double>> x;
	vector<vector<double>> u;

public:
	PIDControllerOrigin(vector<vector<double>> AB, vector<vector<double>> CD, vector<vector<double>> x_init) {
		this->AB = AB;
		this->CD = CD;
		this->x = x_init;
	}

	vector<vector<double>> Step(vector<vector<double>> y) {
		vector<vector<double>> xy = mergeByRow(x, y);
		u = multPlain(CD, xy);
		x = multPlain(AB, xy);
		return u;
	}
	vector<vector<double>> mergeByRow(vector<vector<double>> a, vector<vector<double>> b) {
		vector<vector<double>> result(a.size() + b.size(), vector<double>(a[0].size(), 0));
		for (int i = 0;i < a.size();i++) {
			for (int j = 0;j < a[0].size();j++) {
				result[i][j] = a[i][j];
			}
		}
		for (int i = 0;i < b.size();i++) {
			for (int j = 0;j < b[0].size();j++) {
				result[a.size() + i][j] = b[i][j];
			}
		}
		return result;
	}
	vector<vector<double>> multPlain(vector<vector<double>> mtx_left, vector<vector<double>> mtx_right) {
		vector<vector<double>> result(mtx_left.size(), vector<double>(mtx_right[0].size(), 0));
		for (int i = 0;i < mtx_left.size(); i++) {
			for (int j = 0;j < mtx_right[0].size();j++) {
				for (int k = 0;k < mtx_right.size();k++) {
					result[i][j] += mtx_left[i][k] * mtx_right[k][j];
				}
			}
		}
		return result;
	}
};
class EncrypterDecrypter {
private:
	vector<unsigned __int64> secretKey;
	unsigned __int64 mod_s;
	unsigned __int64 mod_q; // ciphertext modulus
	unsigned __int64 mod_p; // plaintext modulus
	unsigned __int64 mod_e;
	int n;
	int n_;

	int r;
	int L;
	int L_dividedby_r;
	int S_B;
	int S_CD;
	int M;
	int nu;
	int d;
	int p;
	int q;

	unsigned __int64 mod_qe;
	unsigned __int64 mod_qp;
	unsigned __int64 nu_;
	unsigned __int64 mod_q_;
	unsigned __int64 mod_p_;

public:
	EncrypterDecrypter(int r, int S_B, int S_CD, int M, int L, int mod_e) {
		srand(time(NULL));

		//n = 249; // ciphertext dimension
		n = 1;
		n_ = n + 1;
		mod_s = 256; // secret key range

		for (int i = 0;i < n;i++) {
			secretKey.push_back(rand() % mod_s);
		}

		q = 48;
		mod_q = pow(2, q); // ciphertext modulus

		this->r = r; // signal resolution
		this->L = L; // scaling factor for signal
		this->L_dividedby_r = L / r;
		this->S_B = S_B; // scaling factor for matrix B
		this->S_CD = S_CD; // scaling factor for matrix C and D
		this->M = M * L_dividedby_r;
		p = ceil(log2(this->M) + log2(r) + log2(S_B) + log2(S_CD));
		printf("q=%d, p=%d\n", q, p);
		p = q;
		mod_p = pow(2, p); // plaintext modulus
		cout << "p=" << p << endl;

		nu = 16;

		d = ceil(q / nu);

		this->mod_e = mod_e; // error range -> odd number
		mod_qe = mod_q - (mod_e - 1) / 2;
		mod_qp = pow(2, q - p);
		mod_q_ = mod_q - 1;
		mod_p_ = mod_p - 1;
		nu_ = pow(2, nu) - 1;
	}
	vector<vector<double>> Dec(vector<vector<unsigned __int64>> c, int scaling, bool signal) {
		int l = c.size() / n_;
		vector<vector<double>> y(l, vector<double>(c[0].size(), 0));

		for (int j = 0;j < c[0].size();j++) {
			for (int i = 0;i < l;i++) {
				unsigned __int64 temp = c[i*n_][j];
				for (int k = 0;k < n;k++) {
					temp += secretKey[k] * c[i*n_ + k + 1][j];
				}
				unsigned __int64 m = (unsigned __int64)round(temp / (double)mod_qp) & (mod_p_);
				signed __int64 real_m = m - (m >= mod_p / 2) * mod_p;

				if (signal)
					y[i][j] = (double)real_m / (double)L / (double)scaling;
				else
					y[i][j] = real_m / (double)r / (double)scaling;
			}
		}
		return y;
	}
	vector<vector<double>> Dec(vector<vector<unsigned __int64>> c) {
		return Dec(c, 1, false);
	}

	vector<vector<double>> Dec_signal(vector<vector<unsigned __int64>> c) {
		return Dec(c, S_B*S_CD, true);
	}

	vector<vector<unsigned __int64>> Enc(vector<vector<double>> m, bool signal) {
		vector<vector<unsigned __int64>> c(n_*m.size(), vector<unsigned __int64>(m[0].size(), 0));

		for (int i_m = 0;i_m < m.size();i_m++) {
			// a
			vector<vector<unsigned __int64>> a(n, vector<unsigned __int64>(m[0].size(), 0));

			for (int i_a = 0; i_a < n; i_a++) {
				for (int j_a = 0;j_a < m[0].size();j_a++) {
					a[i_a][j_a] = distribution(generator) % mod_q;
					c[i_m*n_ + (i_a + 1)][j_a] = a[i_a][j_a];
				}
			}

			// b
			vector<unsigned __int64> s_mult_a = mult(secretKey, a);
			for (int j_c = 0;j_c < m[0].size();j_c++) {
				int e = rand() % mod_e + mod_qe;
				int quantized = round(m[i_m][j_c] * r);
				if (signal) {
					unsigned __int64 scaled = L_dividedby_r * quantized;
					c[i_m*n_][j_c] = (unsigned __int64)(mod_q - s_mult_a[j_c] + mod_qp * (scaled) + e) & mod_q_;
				}
				else
					c[i_m*n_][j_c] = (unsigned __int64)(mod_q - s_mult_a[j_c] + mod_qp * (quantized) + e) & mod_q_;
				//c[i_m*n_][j_c] = (unsigned __int64)(mod_q - s_mult_a[j_c] + quantized + e) & mod_q_;
			}

		}
		/*if (signal) {
			for (int i = 0;i < c.size(); i++) {
				for (int j = 0;j < c[0].size();j++) {
					c[i][j] = (L_dividedby_r * c[i][j]) & mod_q_;
				}
			}
		}*/
		return c;
	}
	vector<vector<unsigned __int64>> Encm(vector<vector<double>> m) {
		vector<vector<unsigned __int64>> c(n_ * m.size(), vector<unsigned __int64>(n_ * m[0].size() * d, 0));
		vector<vector<unsigned __int64>> m_temp(m.size(), vector<unsigned __int64>(m[0].size(), 0));
		for (int i = 0;i < m.size(); i++) {
			for (int j = 0;j < m[0].size();j++) {
				m_temp[i][j] = mod_q + round(m[i][j]);
			}
		}

		int piece_start = 0;
		vector<vector<double>> zeros(1, vector<double>(n_, 0));
		vector<vector<unsigned __int64>> enc_zeros;

		for (int piece = 0;piece < d;piece++) {
			for (int i_m = 0;i_m < m.size();i_m++) {
				for (int j_m = 0;j_m < m[0].size();j_m++) {
					enc_zeros = Enc(zeros, false);
					for (int i_c = 0;i_c < n_;i_c++) {
						int i_c_real = i_m * n_ + i_c;
						for (int j_c = 0;j_c < n_;j_c++) {
							int j_c_real = piece_start + j_m * n_ + j_c;
							c[i_c_real][j_c_real] = (c[i_c_real][j_c_real] + (i_c == j_c) * m_temp[i_m][j_m]) & mod_q_;
							c[i_c_real][j_c_real] = (c[i_c_real][j_c_real] + enc_zeros[i_c][j_c]) & mod_q_;
						}
					}

				}
			}

			for (int i_m = 0;i_m < m.size(); i_m++) {
				for (int j_m = 0;j_m < m[0].size();j_m++) {
					m_temp[i_m][j_m] = (m_temp[i_m][j_m] << nu) & mod_q_;
				}
			}
			piece_start = piece_start + (n_)* m[0].size();
		}

		return c;
	}
	vector<unsigned __int64> mult(vector<unsigned __int64> c, vector<vector<unsigned __int64>> CD) {
		vector<unsigned __int64> y(CD[0].size(), 0);

		int size_i = CD.size();
		int size_ii = CD[0].size();

		for (int i = 0;i < size_i;i++) {
			unsigned __int64 c_i = c[i];
			for (int ii = 0;ii < size_ii;ii++) {
				y[ii] = (y[ii] + c_i * CD[i][ii]) & mod_q_;
			}
		}

		return y;
	}
	int delta_Enc() {
		return round((mod_e - 1) / 2 / mod_qp);
	}
	int delta_Mult() {
		return round(d*(n + 1)*(mod_e - 1) / 2 / mod_qp * nu) + 1;
	}
};
class PIDController {

private:
	vector<vector<unsigned __int64>> encm_AB;
	vector<vector<unsigned __int64>> encm_CD;
	const int q = 48;
	const unsigned __int64 mod_q_ = pow(2, q) - 1;
	const int nu = 16;
	const int d = q / nu;
	const unsigned __int64 nu_ = pow(2, nu) - 1;

public:
	vector<vector<unsigned __int64>> enc_x;
	PIDController(vector<vector<unsigned __int64>> encm_AB, vector<vector<unsigned __int64>> encm_CD, vector<vector<unsigned __int64>> enc_x_init) {
		this->encm_AB = encm_AB;
		this->encm_CD = encm_CD;
		this->enc_x = enc_x_init;
	}
	
	vector<vector<unsigned __int64>> Step(vector<vector<unsigned __int64>> enc_y) {
		vector<vector<unsigned __int64>> enc_xy = mergeByRow(enc_x, enc_y);
		vector<vector<unsigned __int64>> split_enc_xy = splitm(enc_xy);
		//startCount();
		vector<vector<unsigned __int64>> enc_u = mult(encm_CD, split_enc_xy);  // controller output
		//printInterval();

		//startCount();
		enc_x = mult(encm_AB, split_enc_xy); // controller state
		//printInterval();
		return enc_u;
	}
	vector<vector<unsigned __int64>> mergeByRow(vector<vector<unsigned __int64>> a, vector<vector<unsigned __int64>> b) {
		vector<vector<unsigned __int64>> result(a.size() + b.size(), vector<unsigned __int64>(a[0].size(), 0));
		for (int i = 0;i < a.size();i++) {
			for (int j = 0;j < a[0].size();j++) {
				result[i][j] = a[i][j];
			}
		}
		for (int i = 0;i < b.size();i++) {
			for (int j = 0;j < b[0].size();j++) {
				result[a.size() + i][j] = b[i][j];
			}
		}
		return result;
	}
	vector<vector<unsigned __int64>> mult(vector<vector<unsigned __int64>> encm, vector<vector<unsigned __int64>> split_enc) {
		int size_i = encm.size();
		int size_k = split_enc.size();
		int size_j = split_enc[0].size();
		//cout << "size_j "<<size_j << endl;
		vector<unsigned __int64> result(size_i*size_j, 0);
		for (int i = 0; i < size_i;i++) {
			vector<unsigned __int64> encm_i = encm[i];
			for (int k = 0;k < size_k;k++) {
				for (int j = 0;j < size_j;j++) {
					result[size_j * i + j] = (result[size_j * i + j] + encm_i[k] * split_enc[k][j]) & mod_q_;
				}
			}
		}
		vector<vector<unsigned __int64>> result_2d(size_i, vector<unsigned __int64>(size_j, 0));
		for (int i = 0; i < size_i;i++) {
			for (int j = 0;j < size_j;j++) {
				result_2d[i][j] = result[size_j * i + j];
			}
		}
		return result_2d;
	}
	vector<vector<unsigned __int64>> splitm(vector<vector<unsigned __int64>> c) {
		vector<vector<unsigned __int64>> result(c.size() * d, vector<unsigned __int64>(c[0].size(), 0));
		vector<vector<unsigned __int64>> c_temp(c.size(), vector<unsigned __int64>(c[0].size(), 0));
		for (int i = 0;i < c.size(); i++) {
			for (int j = 0;j < c[0].size();j++) {
				c_temp[i][j] = c[i][j];
			}
		}

		for (int piece = 0;piece < d;piece++) {
			for (int i = 0;i < c.size();i++) {
				for (int j = 0;j < c[0].size();j++) {
					result[piece * c.size() + i][j] = c_temp[i][j] & nu_;
					c_temp[i][j] = c_temp[i][j] >> (nu);
				}
			}
		}
		return result;
	}
};

class Plant {

private:
	vector<vector<double>> AB;
	vector<vector<double>> CD;
	vector<vector<double>> x;
	vector<vector<double>> x_prime;
	vector<vector<double>> u;
	vector<vector<double>> u_prime;
	vector<vector<double>> y;
	vector<vector<double>> y_prime;
	vector<vector<double>> r;
	int S_B;
	int S_CD;
	int R_y;
	int M;
	int L;
	PIDController *controller;
	PIDControllerOrigin *controllerOrigin;
	EncrypterDecrypter *encdec;

public:
	Plant() {
		string filePath = "parameters.txt";
		// read File
		ifstream ifs;
		ifs.open(filePath.data());
		if (!ifs.is_open()) {
			cout << "No input file" << endl;
			return;
		}
		string line;
		int i = 0;
		while (getline(ifs, line, '\n')) {
			if (line.compare("CD") == 0)
				break;
			AB.push_back(vector<double>());
			size_t pos = 0;
			string token;
			while ((pos = line.find('\t')) != std::string::npos) {
				token = line.substr(0, pos);
				//cout << stod(token) << " ";
				AB[i].push_back(stod(token));
				line.erase(0, pos + 1);
			}
			token = line;
			//cout << stod(token) << endl;
			AB[i].push_back(stod(token));
			i++;
		}
		i = 0;
		while (getline(ifs, line, '\n')) {
			if (line.compare("x") == 0)
				break;
			CD.push_back(vector<double>());
			size_t pos = 0;
			string token;
			while ((pos = line.find('\t')) != std::string::npos) {
				token = line.substr(0, pos);
				//cout << stod(token) << " ";
				CD[i].push_back(stod(token));
				line.erase(0, pos + 1);
			}
			token = line;
			//cout << stod(token) << endl;
			CD[i].push_back(stod(token));
			i++;
		}
		i = 0;
		while (getline(ifs, line, '\n')) {
			if (line.compare("u") == 0)
				break;
			x.push_back(vector<double>());
			size_t pos = 0;
			string token;
			while ((pos = line.find('\t')) != std::string::npos) {
				token = line.substr(0, pos);
				//cout << stod(token) << " ";
				x[i].push_back(stod(token));
				line.erase(0, pos + 1);
			}
			token = line;
			//cout << stod(token) << endl;
			x[i].push_back(stod(token));
			i++;
		}
		x_prime = x;
		i = 0;
		while (getline(ifs, line, '\n')) {
			if (line.compare("y") == 0)
				break;
			u.push_back(vector<double>());
			size_t pos = 0;
			string token;
			while ((pos = line.find('\t')) != std::string::npos) {
				token = line.substr(0, pos);
				//cout << stod(token) << " ";
				u[i].push_back(stod(token));
				line.erase(0, pos + 1);
			}
			token = line;
			//cout << stod(token) << endl;
			u[i].push_back(stod(token));
			i++;
		}
		u_prime = u;
		i = 0;
		while (getline(ifs, line, '\n')) {
			if (line.compare("r") == 0)
				break;
			y.push_back(vector<double>());
			size_t pos = 0;
			string token;
			while ((pos = line.find('\t')) != std::string::npos) {
				token = line.substr(0, pos);
				//cout << stod(token) << " ";
				y[i].push_back(stod(token));
				line.erase(0, pos + 1);
			}
			token = line;
			//cout << stod(token) << endl;
			y[i].push_back(stod(token));
			i++;
		}
		y_prime = y;
		i = 0;
		while (getline(ifs, line, '\n')) {
			if (line.compare("PID") == 0)
				break;
			r.push_back(vector<double>());
			size_t pos = 0;
			string token;
			while ((pos = line.find('\t')) != std::string::npos) {
				token = line.substr(0, pos);
				//cout << stod(token) << " ";
				r[i].push_back(stod(token));
				line.erase(0, pos + 1);
			}
			token = line;
			//cout << stod(token) << endl;
			r[i].push_back(stod(token));
			i++;
		}
		getline(ifs, line, '\n');
		//cout << line << endl;
		size_t pos = 0;
		pos = line.find('\t');
		double k_p = stod(line.substr(0, pos));
		line.erase(0, pos + 1);
		pos = line.find('\t');
		double k_i = stod(line.substr(0, pos));
		line.erase(0, pos + 1);
		pos = line.find('\t');
		double k_d = stod(line.substr(0, pos));
		line.erase(0, pos + 1);
		pos = line.find('\t');
		int N_d = stoi(line.substr(0, pos));
		line.erase(0, pos + 1);
		double T_s = stod(line);

		ifs.close();

		//printf("plant initial state x=\n");
		//print(x);

		//R_y = pow(2,9); // scaling factor for signal
		R_y = pow(2,10); // scaling factor for signal
		S_B = 1; // scaling factor for controller matrix B
		//S_CD = pow(2,9); // scaling factor for controller matrices C and D
		S_CD = pow(2,10);
		M = pow(2,11); // plaintext range for signal y, u, and controller state x
		L = pow(2,26);
		int mod_e = 7;
		encdec = new EncrypterDecrypter(R_y, S_B, S_CD, M, L, mod_e);

		print(encdec->Dec(encdec->Enc(x, true), R_y, true));

		cout << "delta Enc: " << encdec->delta_Enc() << " delta Mult: " << encdec->delta_Mult() << endl;

		buildPIDController(k_p, k_i, k_d, N_d, T_s);
		//buildExerciseController();
	}

	void buildPIDController(double k_p, double k_i, double k_d, int N_d, double T_s) {
		vector<vector<double>> A = { { 2 - (double)N_d, (double)N_d - 1 },{ 1,0 } };
		vector<vector<double>> B = { { 1 },{ 0 } };
		int x_row = A.size();
		int y_row = B[0].size();
		vector<vector<double>> AB = vector<vector<double>>(x_row, vector<double>(x_row + y_row));
		for (int row = 0;row < x_row;row++) {
			for (int col = 0;col < x_row;col++) {
				AB[row][col] = A[row][col];
			}
			for (int col = x_row;col < x_row + y_row;col++) {
				AB[row][col] = B[row][col - x_row];
			}
		}
		vector<vector<double>> AB_scaled = vector<vector<double>>(x_row, vector<double>(x_row + y_row));
		for (int row = 0;row < x_row;row++) {
			for (int col = 0;col < x_row;col++) {
				AB_scaled[row][col] = AB[row][col];
			}
			for (int col = x_row;col < x_row + y_row;col++) {
				AB_scaled[row][col] = round(AB[row][col] * S_B);
			}
		}

		vector<vector<double>> C = { { k_i * T_s - k_d * N_d * N_d / T_s, k_i * T_s * N_d - k_i * T_s + k_d * N_d * N_d / T_s } };
		vector<vector<double>> D = { { k_p + k_d * N_d / T_s } };
		int u_row = C.size();
		vector<vector<double>> CD = vector<vector<double>>(u_row, vector<double>(x_row + y_row));
		for (int row = 0;row < u_row;row++) {
			for (int col = 0;col < x_row;col++) {
				CD[row][col] = C[row][col];
			}
			for (int col = x_row;col < x_row + y_row;col++) {
				CD[row][col] = D[row][col - x_row];
			}
		}
		vector<vector<double>> CD_scaled = vector<vector<double>>(u_row, vector<double>(x_row + y_row));
		for (int row = 0;row < u_row;row++) {
			for (int col = 0;col < x_row;col++) {
				CD_scaled[row][col] = round(CD[row][col] * S_CD);
			}
			for (int col = x_row;col < x_row + y_row;col++) {
				CD_scaled[row][col] = round(CD[row][col] * S_CD * S_B);
			}
		}

		vector<vector<unsigned __int64>> encm_AB = encdec->Encm(AB_scaled);
		vector<vector<unsigned __int64>> encm_CD = encdec->Encm(CD_scaled);

		vector<vector<double>> x_init_con = { { 0 }, { 0 } };
		vector<vector<unsigned __int64>> enc_x_init_con = encdec->Enc(x_init_con, false);

		controllerOrigin = new PIDControllerOrigin(AB, CD, x_init_con);
		controller = new PIDController(encm_AB, encm_CD, enc_x_init_con);

		double B_norm = getInfiniteNorm(B);
		double D_norm = getInfiniteNorm(D);
		cout << B_norm << " " << D_norm << endl;
		cout << "(Beta1 - Alpha1) / Alpha1 = " << (B_norm * encdec->delta_Enc() / L + (x.size() + y.size() + r.size()) * encdec->delta_Mult() / L / S_B) / (R_y / 2 * (B_norm)) << endl;
		cout << "(Beta2 - Alpha2) / Alpha2 = " << (D_norm * encdec->delta_Enc() / L + (x.size() + y.size() + r.size()) * encdec->delta_Mult() / L / S_B / S_CD) / (R_y / 2 * (D_norm + 1)) << endl;
		cout << "(Beta3 - Alpha3) / Alpha3 = " << 2 * encdec->delta_Enc() / L << endl;
	}

	void getOutput() {
		vector<vector<double>> xu = mergeByRow(x, u);
		y = multPlain(CD, xu); // plant output
		vector<vector<double>> xu_prime = mergeByRow(x_prime, u_prime);
		y_prime = multPlain(CD, xu_prime);
	}

	void updateState() {
		vector<vector<double>> xu = mergeByRow(x, u);
		x = multPlain(AB, xu); // plant state
		vector<vector<double>> xu_prime = mergeByRow(x_prime, u_prime);
		x_prime = multPlain(AB, xu_prime);
	}

	void loop() {
		startCount();
		for (int t = 0;t <= 3000;t++) {
			getOutput();
			/*if (t % 100 == 0) {
				printf("t=%d\n", t);
				cout << "r-y=" << endl;
				print(Substraction(r, y));
				//startCount();
			}*/
			u = encdec->Dec_signal(controller->Step(encdec->Enc(Substraction(r, y), true)));
			u_prime = controllerOrigin->Step(Substraction(r, y_prime));
			updateState();
		}
		cout << "y-y_prime=" << endl;
		print(Substraction(y, y_prime));
		print(Substraction(r, y_prime));
		errors.push_back(Substraction(y, y_prime)[0][0]);
		//printInterval();
	}

	vector<vector<double>> mergeByRow(vector<vector<double>> a, vector<vector<double>> b) {
		vector<vector<double>> result(a.size() + b.size(), vector<double>(a[0].size(), 0));
		for (int i = 0;i < a.size();i++) {
			for (int j = 0;j < a[0].size();j++) {
				result[i][j] = a[i][j];
			}
		}
		for (int i = 0;i < b.size();i++) {
			for (int j = 0;j < b[0].size();j++) {
				result[a.size() + i][j] = b[i][j];
			}
		}
		return result;
	}
	vector<vector<double>> multPlain(vector<vector<double>> mtx_left, vector<vector<double>> mtx_right) {
		vector<vector<double>> result(mtx_left.size(), vector<double>(mtx_right[0].size(), 0));
		for (int i = 0;i < mtx_left.size(); i++) {
			for (int j = 0;j < mtx_right[0].size();j++) {
				for (int k = 0;k < mtx_right.size();k++) {
					result[i][j] += mtx_left[i][k] * mtx_right[k][j];
				}
			}
		}
		return result;
	}
	vector<vector<double>> Substraction(vector<vector<double>> mtx_left, vector<vector<double>> mtx_right) {
		vector<vector<double>> result(mtx_left.size(), vector<double>(mtx_left[0].size(), 0));
		for (int i = 0;i < mtx_left.size(); i++) {
			for (int j = 0;j < mtx_left[0].size();j++) {
				result[i][j] = mtx_left[i][j] - mtx_right[i][j];
			}
		}
		return result;
	}
	double getInfiniteNorm(vector<vector<double>> m) {
		double max = 0;
		for (int i = 0;i < m.size();i++) {
			double sum = 0;
			for (int j = 0;j < m[i].size();j++) {
				sum += abs(m[i][j]);
			}
			if (sum > max)
				max = sum;
		}
		return max;
	}
};

//------------------------------------
void LWECheck() {
	EncrypterDecrypter *encdec = new EncrypterDecrypter(32, 1, 1, 16384, 1, 7);

	vector<vector<double>> A = { {1.25, 0.125, -10}, {-100, -5, 1000} };
	printf("A:\n");
	print(A);

	vector<vector<double>> B = { {4, 521, -10}, {150, 1, 2000} };
	printf("B:\n");
	print(B);

	vector<vector<unsigned __int64>> A_enc = encdec->Enc(A, false);
	vector<vector<unsigned __int64>> B_enc = encdec->Enc(B, false);
	vector<vector<unsigned __int64>> A_enc_mult_scalar = scalar_mult(3, A_enc);
	vector<vector<unsigned __int64>> A_enc_add_B_enc = add_enc(A_enc, B_enc);
	printf("Enc(A):\n");
	print(A_enc);
	vector<vector<double>> dec1 = encdec->Dec(A_enc);
	printf("Dec(Enc(A)):\n");
	print(dec1);
	vector<vector<double>> dec2 = encdec->Dec(A_enc_add_B_enc);
	printf("Dec(Enc(A) + Enc(B)):\n");
	print(dec2);
	vector<vector<double>> dec3 = encdec->Dec(A_enc_mult_scalar);
	printf("Dec(Enc(A)*3):\n");
	print(dec3);
}
void GSWCheck() {
	EncrypterDecrypter *encdec = new EncrypterDecrypter(8, 1, 1, 16384, 1, 7);

	int AB_row = 2;
	int AB_col = 3;
	vector<vector<double>> AB = { { -4, 3, 1 },{ 4,11,0 } };
	int xy_row = AB_col;
	int xy_col = 2;
	vector<vector<double>> xu = { { -5, 1 },{ -2.5, 0.125 },{ 2, 0 } };

	printf("AB:\n");
	print(AB);
	printf("xu:\n");
	print(xu);

	vector<vector<unsigned __int64>> encm_AB = encdec->Encm(AB);
	vector<vector<unsigned __int64>> enc_xu = encdec->Enc(xu, false);
	vector<vector<unsigned __int64>> split_enc_xu = splitm(enc_xu);
	vector<vector<unsigned __int64>> enc_mult = mult(encm_AB, split_enc_xu);
	vector<vector<double>> answer = encdec->Dec(enc_mult);
	printf("Dec(Enc(AB) x Enc(xu)):\n");
	print(answer);
}

int main()
{
	srand(time(NULL));
	//LWECheck();
	//GSWCheck();
	int step = 10;
	double total = 0;
	for (int i = 0;i < step;i++) {
		Plant* plant = new Plant();
		plant->loop();
		total += abs(errors.back());
	}
	cout << (total/(double)step) << endl;
    return 0;
}
