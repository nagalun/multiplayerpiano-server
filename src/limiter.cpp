#include "limiter.hpp"

bool limiter::Bucket::can_spend(uint16_t count){
	auto now = std::chrono::steady_clock::now();
	std::chrono::duration<float> passed = now - last_check;
	last_check = now;
	allowance += passed.count() * ((float)rate / per);
	if(allowance > rate)
		allowance = rate;
	if(allowance < count)
		return false;
	allowance -= count;
	return true;
}

bool limiter::Simple::can_spend(){
	auto now = std::chrono::steady_clock::now();
	std::chrono::duration<float> passed = now - last_check;
	if(passed.count() < rate)
		return false;
	last_check = now;
	return true;
}
