#ifndef H_LMA_H
#define H_LMA_H
//http://people.duke.edu/~hpgavin/ce281/lm.pdf
#include "MultiVariableFunction.h"
#include "Matrix.h"
#include<vector>
class LevenbergMarquardtAlgorithm {
protected:
	//_foo ->foo member func of class
	//foo_ ->foo should not be deleted (created on heap outside class or is returned to user at some point)
	bool print = false;
	MultiVariableFunction* _func;
	unsigned int _nParams = 0;//no of params, passed in constructor
	unsigned int _nPoints = 0;//number of data points, passed in run() args
	unsigned int _currentIterations = 0;
	unsigned int _maxIterations = 1000;
	bool _canDelete = true;
	bool _initialised = false;
	double _lambda = 0.95f;//damping parameter
	double _v = 0.95f;
	double lastSum = FLT_MAX;
	double* _currentParams_;
	double* _initGuess_;//initial parameter guess; passed in constructor
	double* _partialDerivatives;//array of partial derivatives
	double** _data_;//passed in constructor
	double** _funcValues;
	double** _jacobian;
	Matrix *_matrixJ, *_matrixJT, *_matrixDiagJTJ, *_matrixYF, *_matrixLHS, *_matrixRHS, *_matrixDelta;
	std::vector<Matrix*> matrixArr = { _matrixJ,_matrixJT,_matrixDiagJTJ,_matrixYF,_matrixLHS,_matrixRHS,_matrixDelta };
	void deleteAll() {
		if (!_canDelete) {
			return;
		}

		printStatus("DeleteAll");
		delete[] _partialDerivatives;
		delete[] _funcValues;
		delete2DArray(_jacobian, _nPoints);
		for (Matrix* a : matrixArr) {
			delete a;
		}

	}
	void delete2DArray(double** arr, unsigned int height) {
		printStatus("Delete2DArray");
		for (unsigned int i = 0; i < height; i++) {
			delete[] arr[i];
		}
		delete[] arr;
	}
	void initialise() {
		printStatus("initialise");
		_initialised = true;
		_partialDerivatives = new double[_nParams];
		_currentParams_ = new double[_nParams];
		_jacobian = new double*[_nPoints];
		_funcValues = new double*[1];
		_funcValues[0] = new double[_nPoints];
		for (unsigned int i = 0; i < _nPoints; i++) {
			_jacobian[i] = new double[_nParams];
			_funcValues[0][i] = 0;
			for (unsigned int j = 0; j < _nParams; j++) {
				_jacobian[i][j] = 0;
			}
		}
		for (unsigned int i = 0; i < _nParams; i++) {
			_currentParams_[i] = _initGuess_[i];
			_partialDerivatives[i] = 0;
		}
	}
	void adjustParameters() {
		printStatus("adjustParameters");
		for (unsigned int i = 0; i < _nParams; i++) {
			_currentParams_[i] += _matrixDelta->get(0,i);
		}
	}
	void formJacobian() {
		printStatus("formJacobian");
		for (unsigned int i = 0; i < _nPoints; i++) {
			_funcValues[0][i] = _data_[1][i] - _func->getValue(_currentParams_, _data_[0][i]);
			_partialDerivatives = _func->getJacobian(_currentParams_, _partialDerivatives, _data_[0][i]);
			for (unsigned int j = 0; j < _nParams; j++) {
				_jacobian[i][j] = _partialDerivatives[j];
			}
		}
	}
	void runAlgorithm() {	
		printStatus("runAlgorithm");
		_matrixJT = new Matrix(_nPoints, _nParams);		//
		_matrixJ = new Matrix(_nParams, _nPoints);		//
		_matrixDiagJTJ = new Matrix(_nParams, _nParams);	//formed from lambda*diag(JT*J)
		_matrixYF = new Matrix(1, _nPoints);
		_matrixLHS = new Matrix(_nParams, _nParams);		//formed from (JT*J - lambda*diag(JT*J)).inverse
		_matrixRHS = new Matrix(1, _nParams);
		_matrixDelta = new Matrix(1, _nParams);
		
		_currentIterations = 0;
		while (true) {
			_matrixDelta = getAdjustments();
			adjustParameters();
			//_matrixDelta->printMatrix("Delta");
			if (checkSquareSum()) {
				break;
			}
		}
		std::cout << "====================\nResults\n====================" << std::endl;
		printArray(_currentParams_, _nParams);
	}
	bool checkSquareSum() {
		_currentIterations++;
		double temp = _matrixYF->getSquareSum();
		if (temp < lastSum) {
			_lambda *= _v;
		}
		else {
			_lambda /= _v;
		}
		lastSum = temp;
		if (_currentIterations > _maxIterations) {
			return true;
		}
		return false;
	}
	Matrix* getLHS() {
		//return the left hand side of the LMA equation (JT * J  +  lambda*diag(JT*J))
		printStatus("getLHS");
		_matrixJ->copyMatrix(_jacobian,_nParams,_nPoints);
		_matrixJT->copyMatrixTranspose(_matrixJ);
		_matrixLHS->fromMatrixMultiplication(_matrixJT, _matrixJ);
		_matrixDiagJTJ->copyMatrix(_matrixLHS);
		_matrixDiagJTJ->diagonalise();
		(*_matrixDiagJTJ) *= _lambda;
		(*_matrixLHS) += _matrixDiagJTJ;
		_matrixLHS->invert();
		return _matrixLHS;
	}
	Matrix* getRHS() {
		//_y_ and _funcValues are double[1][_nPoints] (ie height 1), therefore form from matrix transpose
		printStatus("getRHS");
		_matrixYF->copyMatrixTranspose(_funcValues, _nPoints, 1);
		_matrixRHS->fromMatrixMultiplication(_matrixJT, _matrixYF);
		return _matrixRHS;
	}
	Matrix* getAdjustments() {
		formJacobian();
		_matrixLHS = getLHS();
		_matrixRHS = getRHS();
		_matrixDelta->fromMatrixMultiplication(_matrixLHS, _matrixRHS);
		/*_matrixJ->printMatrix("J");
		_matrixJT->printMatrix("J^T");
		_matrixDiagJTJ->printMatrix("Diag");
		_matrixLHS->printMatrix("LHS");
		_matrixYF->printMatrix("Y-F");
		_matrixRHS->printMatrix("RHS");
		_matrixDelta->printMatrix("Delta");
		*/
		return _matrixDelta;
	}
	void printStatus(std::string str) {
		if (print) {
			std::cout << str.c_str() << std::endl;
		}
	}
	void printArray(double* arr, int len) {
		for (int i = 0; i < len; i++) {
			std::cout << arr[i] << "\t";
		}
		std::cout << std::endl;
	}
	void printArray(double** arr, int height, int len) {
		for (int i = 0; i < height; i++) {
			printArray(arr[i], len);
		}
	}
public:
	LevenbergMarquardtAlgorithm(MultiVariableFunction* func):_func(func),_nParams(func->getNParams()) {
	}
	LevenbergMarquardtAlgorithm() {

	}
	~LevenbergMarquardtAlgorithm() {
		printStatus("destructor");
		if (!_canDelete) {
			return;
		}
		deleteAll();
		_canDelete = false;
	}
	double* run(double** data, double* initGuess, unsigned int nPoints) {
		printStatus("run");
		_data_ = data;
		_initGuess_ = initGuess;
		_nPoints = nPoints;
		if (!_initialised) {
			initialise();
		}
		runAlgorithm();
		return _currentParams_;
	}
	void setMaxIteration(unsigned int x) {
		_maxIterations = x;
	}
};
#endif