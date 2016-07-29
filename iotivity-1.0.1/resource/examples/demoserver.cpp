#include <queue>
#include <list>
#include <functional>

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <mutex>
#include <condition_variable>
#include <signal.h>
#include <netinet/in.h>

#include "OCPlatform.h"
#include "OCApi.h"

using namespace OC;
using namespace std;
namespace PH = std::placeholders;

int gObservation = 0;
void CheckButtonRepresentation(void *param);
void * grovepi_thread(void *param);
queue <void (*)(void *)> write_queue;
mutex led_write_lock;
void t_write_led_red(void *);
void t_write_led_green(void *);
void t_write_led_blue(void *);
void * handleSlowResponse (void *param, std::shared_ptr<OCResourceRequest> pRequest);
// Specifies where to notify all observers or list of observers
// false: notifies all observers
// true: notifies list of observers
bool isListOfObservers = false;

// Specifies secure or non-secure
// false: non-secure resource
// true: secure resource
bool isSecure = false;

/// Specifies whether Entity handler is going to do slow response or not
bool isSlowResponse = false;


class DemoResource
{

public:
	std::string demo_name;

	double sensor_temp;
	double sensor_humidity;
	int sensor_light;
	int sensor_sound;
	OCResourceHandle sensor_resourceHandle;
	OCRepresentation sensor_rep;

	int led_red;
	int led_green;
	int led_blue;
	OCResourceHandle led_resourceHandle;
	OCRepresentation led_rep;

	std::string lcd_str;
	OCResourceHandle lcd_resourceHandle;
	OCRepresentation lcd_rep;

	double buzzer;
	OCResourceHandle buzzer_resourceHandle;
	OCRepresentation buzzer_rep;

	int button;
	OCResourceHandle button_resourceHandle;
	OCRepresentation button_rep;

	int ultrasonic;
	OCResourceHandle ultrasonic_resourceHandle;
	OCRepresentation ultrasonic_rep;


	ObservationIds m_interestedObservers;

	//std::string py_path = "/home/u/demo/iotivity/extlibs/GrovePi/Software/Python";

private:
	int sockfd;
#define PACKET_LEN  64

public:
	DemoResource()
		:demo_name("Demo resource"), 
		sensor_temp(0.0), sensor_humidity(0.0), sensor_light(0), sensor_sound(0),
		sensor_resourceHandle(nullptr),
		led_red(0), led_green(0), led_blue(0),
		led_resourceHandle(nullptr),
		lcd_str("LCD Demo"),
		lcd_resourceHandle(nullptr), 
		buzzer(0.0),
		buzzer_resourceHandle(nullptr),
		button(0),
	       	button_resourceHandle(nullptr),
		ultrasonic(0),
		ultrasonic_resourceHandle(nullptr)
	{
		// Initialize representation
		sensor_rep.setUri("/grovepi/sensor");
		sensor_rep.setValue("temperature", sensor_temp);
		sensor_rep.setValue("humidity", sensor_humidity);
		sensor_rep.setValue("light", sensor_light);
		sensor_rep.setValue("sound", sensor_sound);

		led_rep.setUri("/grovepi/led");
		led_rep.setValue("red", led_red);
		led_rep.setValue("green", led_green);
		led_rep.setValue("blue", led_blue);

		led_rep.setUri("/grovepi/lcd");
		led_rep.setValue("lcd", lcd_str);

		led_rep.setUri("/grovepi/buzzer");
		led_rep.setValue("buzzer", buzzer);

		led_rep.setUri("/grovepi/button");
		led_rep.setValue("button", button);

		ultrasonic_rep.setUri("/grovepi/ultrasonic");
		ultrasonic_rep.setValue("ultrasonic", ultrasonic);

		//setenv("PYTHONPATH", py_path.c_str(), 1);
	}

	/* Note that this does not need to be a member function: for classes you do not have
	access to, you can accomplish this with a free function: */

	/// This function internally calls registerResource API.
	void createResource()
	{
		// Sensor resource
		std::string resourceURI;
		std::string resourceTypeName;
		std::string resourceInterface = DEFAULT_INTERFACE;

		// OCResourceProperty is defined ocstack.h
		uint8_t resourceProperty;
		if(isSecure)
			resourceProperty = OC_DISCOVERABLE | OC_OBSERVABLE | OC_SECURE;
		else
			resourceProperty = OC_DISCOVERABLE | OC_OBSERVABLE;

		// Resource callback functions
		EntityHandler sensor_cb = std::bind(&DemoResource::sensor_entityHandler, this,PH::_1);
		EntityHandler led_cb = std::bind(&DemoResource::led_entityHandler, this,PH::_1);
		EntityHandler lcd_cb = std::bind(&DemoResource::lcd_entityHandler, this,PH::_1);
		EntityHandler buzzer_cb = std::bind(&DemoResource::buzzer_entityHandler, this,PH::_1);
		EntityHandler button_cb = std::bind(&DemoResource::button_entityHandler, this,PH::_1);
		EntityHandler ultrasonic_cb = std::bind(&DemoResource::ultrasonic_entityHandler, this,PH::_1);

		// This will internally create and register the resource.
		resourceURI = "/grovepi/sensor";
		resourceTypeName = "grovepi.sensor";
		OCStackResult result = OCPlatform::registerResource(
			sensor_resourceHandle, resourceURI, resourceTypeName,
			resourceInterface, sensor_cb, resourceProperty);

		if (OC_STACK_OK != result)
			cout << "Sensor resource creation was unsuccessful\n";

		// Create LED resource
		resourceURI = "/grovepi/led";
		resourceTypeName = "grovepi.led";
		result = OCPlatform::registerResource(
			led_resourceHandle, resourceURI, resourceTypeName,
			resourceInterface, led_cb, resourceProperty);

		if (OC_STACK_OK != result)
			cout << "LED resource creation was unsuccessful\n";

		// Create LCD resource
		resourceURI = "/grovepi/lcd";
		resourceTypeName = "grovepi.lcd";
		result = OCPlatform::registerResource(
			lcd_resourceHandle, resourceURI, resourceTypeName,
			resourceInterface, lcd_cb, resourceProperty);

		if (OC_STACK_OK != result)
			cout << "LCD resource creation was unsuccessful\n";

		// Create Buzzer resource
		resourceURI = "/grovepi/buzzer";
		resourceTypeName = "grovepi.buzzer";
		result = OCPlatform::registerResource(
			buzzer_resourceHandle, resourceURI, resourceTypeName,
			resourceInterface, buzzer_cb, resourceProperty);

		if (OC_STACK_OK != result)
			cout << "Buzzer resource creation was unsuccessful\n";

		// Create Button resource
		resourceURI = "/grovepi/button";
		resourceTypeName = "grovepi.button";
		result = OCPlatform::registerResource(
			button_resourceHandle, resourceURI, resourceTypeName,
			resourceInterface, button_cb, resourceProperty);

		if (OC_STACK_OK != result)
			cout << "Button resource creation was unsuccessful\n";

		// Create Ultrasonic resource
		resourceURI = "/grovepi/ultrasonic";
		resourceTypeName = "grovepi.ultrasonic";
		result = OCPlatform::registerResource(
			ultrasonic_resourceHandle, resourceURI, resourceTypeName,
			resourceInterface, ultrasonic_cb, resourceProperty);

		if (OC_STACK_OK != result)
			cout << "Ultrasonic resource creation was unsuccessful\n";
	}

	int python_server_connect()
	{
		int portno = 5566;
		struct sockaddr_in serv_addr;
		int result = 0;

		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(portno);
		serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0) {
			std::cout << "ERROR: can not create socket" << std::endl;
			return -1;
		}

		while(1) {
			result = connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr));
			if (result < 0) {
				std::cout << "Retry to connect to python server" << std::endl;
			} else {
				std::cout << "Connection has been built" << std::endl;
				break;
			}

			sleep(2);
		}

		return 0;
	}

	int python_server_read(const char *name, double *value)
	{
		int n;
		char wdata[PACKET_LEN] = {0};
		char data[PACKET_LEN] = {0};

		sprintf(wdata, "%32s", name);
		//std::cout << "data: " << wdata << std::endl;
		n = write(sockfd, wdata, PACKET_LEN);

		n = read(sockfd, data, PACKET_LEN);
		if(n < 0) {
			std::cout << "Read from server failed" << std::endl;
			return 0;
		} else {
			//std::cout <<"Read data: " << data << std::endl;
			*value = std::stod(data);
			return 1;
		}
	}

	int python_server_read(const char *name, int *value)
	{
		int n;
		char wdata[PACKET_LEN] = {0};
		char data[PACKET_LEN] = {0};

		sprintf(wdata, "%32s", name);
		n = write(sockfd, wdata, PACKET_LEN);

		n = read(sockfd, data, PACKET_LEN);
		if(n < 0) {
			std::cout << "Read from server failed" << std::endl;
			return 0;
		} else {
			//std::cout << data << std::endl;
			*value = std::stoi(data);
			return 1;
		}
	}

	int python_server_write(const char *name, double value)
	{
		int n;
		char wdata[PACKET_LEN] = {0};

		sprintf(wdata, "%32s%d", name, (int)value);

		//std::cout << "data: "<< wdata << std::endl;
		n = write(sockfd, wdata, PACKET_LEN);
		if(n < 0) {
			std::cout << "Write to server failed" << std::endl;
			return -1;
		} else {
			return 0;
		} 
	}

	int python_server_write(const char *name, int value)
	{
		int n;
		char wdata[PACKET_LEN] = {0};

		sprintf(wdata, "%32s%d", name, value);

		//std::cout << "data: "<< wdata << std::endl;
		n = write(sockfd, wdata, PACKET_LEN);
		if(n < 0) {
			std::cout << "Write to server failed" << std::endl;
			return -1;
		} else {
			return 0;
		} 
	}

	int python_server_write(const char *name, const char *str)
	{
		int n;
		char wdata[PACKET_LEN+1] = {0};

		sprintf(wdata, "%32s%-32s", name, str);

		//std::cout << "data: "<< wdata << std::endl;
		n = write(sockfd, wdata, PACKET_LEN);
		if(n < 0) {
			std::cout << "Write to server failed" << std::endl;
			return -1;
		} else {
			return 0;
		} 
	}

	double sensor_read_temp()
	{
		const char *name = "sensor_temp";
		double temp;

		if(python_server_read(name, &temp)) {
			return temp;
		} else {
			return -1;
		}
	}

	double sensor_read_humidity()
	{
		const char *name = "sensor_humidity";
		double humidity;

		if(python_server_read(name, &humidity)) {
			return humidity;
		} else {
			return -1;
		}
	}

	int sensor_read_light()
	{
		const char *name = "sensor_light";
		int light;

		if(python_server_read(name, &light)) {
			return light;
		} else {
			return -1;
		}
	}

	int sensor_read_sound()
	{
		const char *name = "sensor_sound";
		int sound;

		if(python_server_read(name, &sound)) {
			return sound;
		} else {
			return -1;
		}
	}

	int led_write_red(int status)
	{
		const char *name = "led_write_red";

		if(python_server_write(name, status)) {
			return 0;
		} else {
			return -1;
		}
	}

	int led_write_green(int status)
	{
		const char *name = "led_write_green";

		if(python_server_write(name, status)) {
			return 0;
		} else {
			return -1;
		}
	}

	int led_write_blue(int status)
	{
		const char *name = "led_write_blue";

		if(python_server_write(name, status)) {
			return 0;
		} else {
			return -1;
		}
	}

	int lcd_write_str(const char *str)
	{
		const char *name = "lcd_write";

		if(strlen(str) > 32) {
			std::cout << "Error: LCD only has 32 chars" << std::endl;
			return -1;
		}

		if(python_server_write(name, str)) {
			return 0;
		} else {
			return -1;
		}
	}

	int buzzer_write(double value)
	{
		const char *name = "buzzer_write";

		if(python_server_write(name, value)) {
			return 0;
		} else {
			return -1;
		}
	}

	int button_read()
	{
		const char *name = "button_read";
		int button;

		if(python_server_read(name, &button)) {
			return button;
		} else {
			return -1;
		}
	}

	int ultrasonic_read()
	{
		const char *name = "ultrasonic_read";
		int ultrasonic;

		if(python_server_read(name, &ultrasonic)) {
			return ultrasonic;
		} else {
			return -1;
		}
	}

	void start_update_thread()
	{
		pthread_t tid;

		std::cout << "Start update thread" << std::endl;
		pthread_create (&tid, NULL, grovepi_thread, (void *)this);
	}

	int update_ip()
	{
		struct ifaddrs *ifaddr, *ifa;
		int s;
		char host_ip[NI_MAXHOST];
		std::string host_str;

		if (getifaddrs(&ifaddr) == -1) {
			std::cout << "getifaddrs error" << std::endl;
			return -1;
		}

		for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
			if (ifa->ifa_addr == NULL)
				continue;

			if(!strcmp(ifa->ifa_name, "eth0") || !strcmp(ifa->ifa_name, "wlan0")) {
				if(ifa->ifa_addr->sa_family == AF_INET) {
					s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host_ip,
						NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
					if(s) {
						std::cout << " getnameinfo fail: " << gai_strerror(s) << std::endl;
						freeifaddrs(ifaddr);
						return -1;
					}
					//host_str = ifa->ifa_name;
					host_str = "IPv4: ";
					host_str += host_ip;
					std::cout << host_str << std::endl;
					break;
				} else if(ifa->ifa_addr->sa_family == AF_INET6) {
					s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in6), host_ip,
						NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
					if(s) {
						std::cout << " getnameinfo fail: " << gai_strerror(s) << std::endl;
						freeifaddrs(ifaddr);
						return -1;
					}
					//host_str = ifa->ifa_name;
					host_str = "IPv6: ";
					host_str += host_ip;
					std::cout << host_str << std::endl;
				}
			}
		}
		freeifaddrs(ifaddr);

		lcd_write_str(host_str.c_str());
		lcd_str = host_str;
		return 0;
	}

	// Puts representation.
	// Gets values from the representation and
	// updates the internal state
	void put_led(OCRepresentation& rep)
	{
		try {
			led_write_lock.lock();
			if (rep.getValue("red", led_red)) {
				cout << "\t\t\t\t" << "Red LED: " << led_red << endl;
				write_queue.push(t_write_led_red);
				//t_write_led_red(this);
			} else {
				cout << "red not found in the representation" << endl;
			}

			if (rep.getValue("green", led_green)) {
				//cout << "\t\t\t\t" << "Green LED: " << led_green << endl;
				write_queue.push(t_write_led_green);
				//t_write_led_green(this);
			} else {
				cout << "green not found in the representation" << endl;
			}

			if (rep.getValue("blue", led_blue)) {
				//cout << "\t\t\t\t" << "Blue LED: " << led_blue << endl;
				write_queue.push(t_write_led_blue);
				//t_write_led_blue(this);
			} else {
				cout << "blue not found in the representation" << endl;
			}
			led_write_lock.unlock();
		}
		catch (exception& e) {
			cout << e.what() << endl;
		}
	}

	void put_lcd(OCRepresentation& rep)
	{
		try {
			if(rep.getValue("lcd", lcd_str)) {
				cout  << "LCD string: " << lcd_str << endl;
				lcd_write_str(lcd_str.c_str());
			} else {
				cout  << "LCD string not found in the representation" << endl;
			}
		}
		catch (exception& e) {
			cout << e.what() << endl;
		}
	}

	void put_buzzer(OCRepresentation& rep)
	{
		try {
			if(rep.getValue("buzzer", buzzer)) {
				cout << "Buzzer: " << buzzer << endl;
				buzzer_write(buzzer);
			} else {
				cout << "buzzer not found in the representation" << endl;
			}
		}
		catch (exception& e) {
			cout << e.what() << endl;
		}
	}

#if 0
	// Post representation.
	// Post can create new resource or simply act like put.
	// Gets values from the representation and
	// updates the internal state
	OCRepresentation post(OCRepresentation& rep)
	{
		static int first = 1;

		// for the first time it tries to create a resource
		if(first)
		{
			first = 0;

			if(OC_STACK_OK == createResource1())
			{
				OCRepresentation rep1;
				rep1.setValue("createduri", std::string("/a/light1"));

				return rep1;
			}
		}

		// from second time onwards it just puts
		put(rep);

		return get_sensor();
	}
#endif


	// gets the updated representation.
	OCRepresentation get_sensor()
	{
		sensor_rep.setValue("temperature", sensor_temp);
		sensor_rep.setValue("humidity", sensor_humidity);
		sensor_rep.setValue("light", sensor_light);
		sensor_rep.setValue("sound", sensor_sound);
		return sensor_rep;
	}

	OCRepresentation get_led()
	{
		led_rep.setValue("red", led_red);
		led_rep.setValue("green", led_green);
		led_rep.setValue("blue", led_blue);
		return led_rep;
	}

	OCRepresentation get_lcd()
	{
		lcd_rep.setValue("lcd", lcd_str);
		return lcd_rep;
	}

	OCRepresentation get_buzzer()
	{
		buzzer_rep.setValue("buzzer", buzzer);
		return buzzer_rep;
	}

	OCRepresentation get_button()
	{
		button_rep.setValue("button", button);
		return button_rep;
	}

	OCRepresentation get_ultrasonic()
	{
		ultrasonic_rep.setValue("ultrasonic", ultrasonic);
		return ultrasonic_rep;
	}

#if 0
	void addType(const std::string& type) const
	{
		OCStackResult result = OCPlatform::bindTypeToResource(sensor_resourceHandle, type);
		if (OC_STACK_OK != result)
			cout << "Binding TypeName to Resource was unsuccessful\n";
	}

	void addInterface(const std::string& interface) const
	{
		OCStackResult result = OCPlatform::bindInterfaceToResource(sensor_resourceHandle, interface);
		if (OC_STACK_OK != result)
			cout << "Binding TypeName to Resource was unsuccessful\n";
	}
#endif

private:
	// This is just a sample implementation of entity handler.
	// Entity handler can be implemented in several ways by the manufacturer
	OCEntityHandlerResult sensor_entityHandler(std::shared_ptr<OCResourceRequest> request)
	{
		cout << "In Server sensor entity handler:\n";
		OCEntityHandlerResult ehResult = OC_EH_ERROR;
		if(request) {
			// Get the request type and request flag
			std::string requestType = request->getRequestType();
			int requestFlag = request->getRequestHandlerFlag();

			if(requestFlag & RequestHandlerFlag::RequestFlag) {
				//cout << "\t\trequestFlag : Request\n";
				auto pResponse = std::make_shared<OC::OCResourceResponse>();
				pResponse->setRequestHandle(request->getRequestHandle());
				pResponse->setResourceHandle(request->getResourceHandle());

				// Check for query params (if any)
				QueryParamsMap queries = request->getQueryParameters();

				if (!queries.empty())
					std::cout << "\nQuery processing upto entityHandler" << std::endl;

				for (auto it : queries) {
					std::cout << "Query key: " << it.first << " value : " << it.second << std:: endl;
				}

				if(requestType == "GET")
				{
					cout << "\trequestType : GET\n";
					pResponse->setErrorCode(200);
					pResponse->setResponseResult(OC_EH_OK);
					pResponse->setResourceRepresentation(get_sensor());
					if(OC_STACK_OK == OCPlatform::sendResponse(pResponse))
						ehResult = OC_EH_OK;
				} else if(requestType == "PUT") {
					cout << "\trequestType : PUT\n";
					cout << "Sensors don't have PUT method" << endl;
				} else if(requestType == "POST") {
					cout << "\trequestType : POST\n";
					cout << "Sensors don't have POST method" << endl;
				} else if(requestType == "DELETE") {
					cout << "\tDelete request received" << endl;
				}
			}

			if(requestFlag & RequestHandlerFlag::ObserverFlag) {
				ObservationInfo observationInfo = request->getObservationInfo();
				if(ObserveAction::ObserveRegister == observationInfo.action) {
					m_interestedObservers.push_back(observationInfo.obsId);
				} else if(ObserveAction::ObserveUnregister == observationInfo.action) {
					m_interestedObservers.erase(std::remove(
							m_interestedObservers.begin(),
							m_interestedObservers.end(),
							observationInfo.obsId),
							m_interestedObservers.end());
				}
			}
		} else {
			std::cout << "Request invalid" << std::endl;
		}

		return ehResult;
	}

	OCEntityHandlerResult led_entityHandler(std::shared_ptr<OCResourceRequest> request)
	{
		cout << "In Server LED entity handler:\n";
		OCEntityHandlerResult ehResult = OC_EH_ERROR;

		if(request) {
			// Get the request type and request flag
			std::string requestType = request->getRequestType();
			int requestFlag = request->getRequestHandlerFlag();

			if(requestFlag & RequestHandlerFlag::RequestFlag) {
				//cout << "\t\trequestFlag : Request\n";
				auto pResponse = std::make_shared<OC::OCResourceResponse>();
				pResponse->setRequestHandle(request->getRequestHandle());
				pResponse->setResourceHandle(request->getResourceHandle());

				// Check for query params (if any)
				QueryParamsMap queries = request->getQueryParameters();

				if (!queries.empty()) {
					std::cout << "\nQuery processing upto entityHandler" << std::endl;
				}

				for (auto it : queries) {
					std::cout << "Query key: " << it.first << " value : " << it.second << std:: endl;
				}

				if(requestType == "GET") {
					cout << "\trequestType : GET\n";
					pResponse->setErrorCode(200);
					pResponse->setResponseResult(OC_EH_OK);
					pResponse->setResourceRepresentation(get_led());
					if(OC_STACK_OK == OCPlatform::sendResponse(pResponse)) {
						ehResult = OC_EH_OK;
					}
				} else if(requestType == "PUT") {
					cout << "\trequestType : PUT\n";
					OCRepresentation rep = request->getResourceRepresentation();

					// Do related operations related to PUT request
					// Update the lightResource
					put_led(rep);
					pResponse->setErrorCode(200);
					pResponse->setResponseResult(OC_EH_OK);
					pResponse->setResourceRepresentation(get_led());
					if(OC_STACK_OK == OCPlatform::sendResponse(pResponse)) {
						ehResult = OC_EH_OK;
					}
				} else if(requestType == "POST") {
					cout << "\trequestType : POST\n";
#if 0
					OCRepresentation rep = request->getResourceRepresentation();

					// Do related operations related to POST request
					OCRepresentation rep_post = post(rep);
					pResponse->setResourceRepresentation(rep_post);
					pResponse->setErrorCode(200);
					if(rep_post.hasAttribute("createduri")) {
						pResponse->setResponseResult(OC_EH_RESOURCE_CREATED);
						pResponse->setNewResourceUri(rep_post.getValue<std::string>("createduri"));
					} else {
						pResponse->setResponseResult(OC_EH_OK);
					}

					if(OC_STACK_OK == OCPlatform::sendResponse(pResponse))
						ehResult = OC_EH_OK;
#endif
				} else if(requestType == "DELETE") {
					cout << "\tDelete request received" << endl;
				}
			}

			if(requestFlag & RequestHandlerFlag::ObserverFlag) {
				ObservationInfo observationInfo = request->getObservationInfo();

				if(ObserveAction::ObserveRegister == observationInfo.action) {
					m_interestedObservers.push_back(observationInfo.obsId);
				} else if(ObserveAction::ObserveUnregister == observationInfo.action) {
					m_interestedObservers.erase(std::remove(
							m_interestedObservers.begin(),
							m_interestedObservers.end(),
							observationInfo.obsId),
							m_interestedObservers.end());
				}
			}
		} else {
			std::cout << "Request invalid" << std::endl;
		}

		return ehResult;
	}

	OCEntityHandlerResult lcd_entityHandler(std::shared_ptr<OCResourceRequest> request)
	{
		cout << "In Server LCD entity handler:\n";
		OCEntityHandlerResult ehResult = OC_EH_ERROR;

		if(request) {
			// Get the request type and request flag
			std::string requestType = request->getRequestType();
			int requestFlag = request->getRequestHandlerFlag();

			if(requestFlag & RequestHandlerFlag::RequestFlag) {
				//cout << "\t\trequestFlag : Request\n";
				auto pResponse = std::make_shared<OC::OCResourceResponse>();
				pResponse->setRequestHandle(request->getRequestHandle());
				pResponse->setResourceHandle(request->getResourceHandle());

				// Check for query params (if any)
				QueryParamsMap queries = request->getQueryParameters();

				if (!queries.empty()) {
					std::cout << "\nQuery processing upto entityHandler" << std::endl;
				}

				for (auto it : queries) {
					std::cout << "Query key: " << it.first << " value : " << it.second << std:: endl;
				}

				if(requestType == "GET") {
					cout << "\trequestType : GET\n";
					pResponse->setErrorCode(200);
					pResponse->setResponseResult(OC_EH_OK);
					pResponse->setResourceRepresentation(get_lcd());
					if(OC_STACK_OK == OCPlatform::sendResponse(pResponse)) {
						ehResult = OC_EH_OK;
					}
				} else if(requestType == "PUT") {
					cout << "\trequestType : PUT\n";
					OCRepresentation rep = request->getResourceRepresentation();

					// Do related operations related to PUT request
					// Update the lightResource
					put_lcd(rep);
					pResponse->setErrorCode(200);
					pResponse->setResponseResult(OC_EH_OK);
					pResponse->setResourceRepresentation(get_lcd());
					if(OC_STACK_OK == OCPlatform::sendResponse(pResponse)) {
						ehResult = OC_EH_OK;
					}
				} else if(requestType == "POST") {
					cout << "\trequestType : POST\n";
#if 0
					OCRepresentation rep = request->getResourceRepresentation();

					// Do related operations related to POST request
					OCRepresentation rep_post = post(rep);
					pResponse->setResourceRepresentation(rep_post);
					pResponse->setErrorCode(200);
					if(rep_post.hasAttribute("createduri")) {
						pResponse->setResponseResult(OC_EH_RESOURCE_CREATED);
						pResponse->setNewResourceUri(rep_post.getValue<std::string>("createduri"));
					} else {
						pResponse->setResponseResult(OC_EH_OK);
					}

					if(OC_STACK_OK == OCPlatform::sendResponse(pResponse))
						ehResult = OC_EH_OK;
#endif
				} else if(requestType == "DELETE") {
					cout << "\tDelete request received" << endl;
				}
			}

			if(requestFlag & RequestHandlerFlag::ObserverFlag) {
				ObservationInfo observationInfo = request->getObservationInfo();

				if(ObserveAction::ObserveRegister == observationInfo.action) {
					m_interestedObservers.push_back(observationInfo.obsId);
				} else if(ObserveAction::ObserveUnregister == observationInfo.action) {
					m_interestedObservers.erase(std::remove(
							m_interestedObservers.begin(),
							m_interestedObservers.end(),
							observationInfo.obsId),
							m_interestedObservers.end());
				}
			}
		} else {
			std::cout << "Request invalid" << std::endl;
		}

		return ehResult;
	}

	OCEntityHandlerResult buzzer_entityHandler(std::shared_ptr<OCResourceRequest> request)
	{
		cout << "In Server buzzer entity handler:\n";
		OCEntityHandlerResult ehResult = OC_EH_ERROR;

		if(request) {
			// Get the request type and request flag
			std::string requestType = request->getRequestType();
			int requestFlag = request->getRequestHandlerFlag();

			if(requestFlag & RequestHandlerFlag::RequestFlag) {
				//cout << "\t\trequestFlag : Request\n";
				auto pResponse = std::make_shared<OC::OCResourceResponse>();
				pResponse->setRequestHandle(request->getRequestHandle());
				pResponse->setResourceHandle(request->getResourceHandle());

				// Check for query params (if any)
				QueryParamsMap queries = request->getQueryParameters();

				if (!queries.empty()) {
					std::cout << "\nQuery processing upto entityHandler" << std::endl;
				}

				for (auto it : queries) {
					std::cout << "Query key: " << it.first << " value : " << it.second << std:: endl;
				}

				if(requestType == "GET") {
					cout << "\trequestType : GET\n";
				} else if(requestType == "PUT") {
					cout << "\trequestType : PUT\n";
					OCRepresentation rep = request->getResourceRepresentation();

					// Do related operations related to PUT request
					// Update the lightResource
					put_buzzer(rep);
					pResponse->setErrorCode(200);
					pResponse->setResponseResult(OC_EH_OK);
					pResponse->setResourceRepresentation(get_buzzer());
					if(OC_STACK_OK == OCPlatform::sendResponse(pResponse)) {
						ehResult = OC_EH_OK;
					}
				} else if(requestType == "POST") {
					cout << "\trequestType : POST\n";
				} else if(requestType == "DELETE") {
					cout << "\tDelete request received" << endl;
				}
			}

			if(requestFlag & RequestHandlerFlag::ObserverFlag) {
				ObservationInfo observationInfo = request->getObservationInfo();

				if(ObserveAction::ObserveRegister == observationInfo.action) {
					m_interestedObservers.push_back(observationInfo.obsId);
				} else if(ObserveAction::ObserveUnregister == observationInfo.action) {
					m_interestedObservers.erase(std::remove(
							m_interestedObservers.begin(),
							m_interestedObservers.end(),
							observationInfo.obsId),
							m_interestedObservers.end());
				}
			}
		} else {
			std::cout << "Request invalid" << std::endl;
		}

		return ehResult;
	}

	OCEntityHandlerResult button_entityHandler(std::shared_ptr<OCResourceRequest> request)
	{
		cout << "In Server button entity handler:\n";
		OCEntityHandlerResult ehResult = OC_EH_ERROR;

		if(request) {
			// Get the request type and request flag
			std::string requestType = request->getRequestType();
			int requestFlag = request->getRequestHandlerFlag();

			if(requestFlag & RequestHandlerFlag::RequestFlag) {
				//cout << "\t\trequestFlag : Request\n";
				auto pResponse = std::make_shared<OC::OCResourceResponse>();
				pResponse->setRequestHandle(request->getRequestHandle());
				pResponse->setResourceHandle(request->getResourceHandle());

				// Check for query params (if any)
				QueryParamsMap queries = request->getQueryParameters();

				if (!queries.empty()) {
					std::cout << "\nQuery processing upto entityHandler" << std::endl;
				}

				for (auto it : queries) {
					std::cout << "Query key: " << it.first << " value : " << it.second << std:: endl;
				}

				if(requestType == "GET") {
					cout << "\trequestType : GET\n";
					pResponse->setErrorCode(200);
					pResponse->setResponseResult(OC_EH_OK);
					pResponse->setResourceRepresentation(get_button());
					if(OC_STACK_OK == OCPlatform::sendResponse(pResponse)) {
						ehResult = OC_EH_OK;
					}
				} else if(requestType == "PUT") {
					cout << "\trequestType : PUT\n";
				} else if(requestType == "POST") {
					cout << "\trequestType : POST\n";
				} else if(requestType == "DELETE") {
					cout << "\tDelete request received" << endl;
				}
			}

			if(requestFlag & RequestHandlerFlag::ObserverFlag) {
				ObservationInfo observationInfo = request->getObservationInfo();

				if(ObserveAction::ObserveRegister == observationInfo.action) {
					cout << "Register observer" << endl;
					m_interestedObservers.push_back(observationInfo.obsId);
					gObservation = 1;
				} else if(ObserveAction::ObserveUnregister == observationInfo.action) {
					cout << "Un-register observer" << endl;
					m_interestedObservers.erase(std::remove(
							m_interestedObservers.begin(),
							m_interestedObservers.end(),
							observationInfo.obsId),
							m_interestedObservers.end());
					gObservation = 0;
				}

				cout << "\trequestFlag : Observer\n";
				ehResult = OC_EH_OK;
			}
		} else {
			std::cout << "Request invalid" << std::endl;
		}

		return ehResult;
	}

	OCEntityHandlerResult ultrasonic_entityHandler(std::shared_ptr<OCResourceRequest> request)
	{
		cout << "In Server button entity handler:\n";
		OCEntityHandlerResult ehResult = OC_EH_ERROR;

		if(request) {
			// Get the request type and request flag
			std::string requestType = request->getRequestType();
			int requestFlag = request->getRequestHandlerFlag();

			if(requestFlag & RequestHandlerFlag::RequestFlag) {
				//cout << "\t\trequestFlag : Request\n";
				auto pResponse = std::make_shared<OC::OCResourceResponse>();
				pResponse->setRequestHandle(request->getRequestHandle());
				pResponse->setResourceHandle(request->getResourceHandle());

				// Check for query params (if any)
				QueryParamsMap queries = request->getQueryParameters();

				if (!queries.empty()) {
					std::cout << "\nQuery processing upto entityHandler" << std::endl;
				}

				for (auto it : queries) {
					std::cout << "Query key: " << it.first << " value : " << it.second << std:: endl;
				}

				if(requestType == "GET") {
					cout << "\trequestType : GET\n";
					pResponse->setErrorCode(200);
					pResponse->setResponseResult(OC_EH_OK);
					pResponse->setResourceRepresentation(get_ultrasonic());
					if(OC_STACK_OK == OCPlatform::sendResponse(pResponse)) {
						ehResult = OC_EH_OK;
					}
				} else if(requestType == "PUT") {
					cout << "\trequestType : PUT\n";
				} else if(requestType == "POST") {
					cout << "\trequestType : POST\n";
				} else if(requestType == "DELETE") {
					cout << "\tDelete request received" << endl;
				}
			}

			if(requestFlag & RequestHandlerFlag::ObserverFlag) {
				ObservationInfo observationInfo = request->getObservationInfo();

				if(ObserveAction::ObserveRegister == observationInfo.action) {
					cout << "Register observer" << endl;
					m_interestedObservers.push_back(observationInfo.obsId);
					gObservation = 1;
				} else if(ObserveAction::ObserveUnregister == observationInfo.action) {
					cout << "Un-register observer" << endl;
					m_interestedObservers.erase(std::remove(
							m_interestedObservers.begin(),
							m_interestedObservers.end(),
							observationInfo.obsId),
							m_interestedObservers.end());
					gObservation = 0;
				}

				cout << "\trequestFlag : Observer\n";
				ehResult = OC_EH_OK;
			}
		} else {
			std::cout << "Request invalid" << std::endl;
		}

		return ehResult;
	}


};


void t_read_temp(void *param)
{
	DemoResource *pdemo = (DemoResource *)param;
	double value = pdemo->sensor_read_temp();

	if(value == -256) {
		cout << "Unable to read temperature sensor" << endl;
	} else {
		//cout << "Update temperature sensor: " << value << endl;
		pdemo->sensor_temp = value;
	}
}	

void t_read_humidity(void *param)
{
	DemoResource *pdemo = (DemoResource *)param;
	double value = pdemo->sensor_read_humidity();

	if(value == -256) {
		cout << "Unable to read humidity sensor" << endl;
	} else {
		//cout << "Update humidity sensor: " << value << endl;
		pdemo->sensor_humidity = value;
	}
}	

void t_read_light(void *param)
{
	DemoResource *pdemo = (DemoResource *)param;
	int value = pdemo->sensor_read_light();

	if(value == -1) {
		cout << "Unable to read light sensor" << endl;
	} else {
		//cout << "Update light sensor: " << value << endl;
		pdemo->sensor_light = value;
	}
}	

void t_read_sound(void *param)
{
	DemoResource *pdemo = (DemoResource *)param;
	int value = pdemo->sensor_read_sound();

	if(value == -1) {
		cout << "Unable to read sound sensor" << endl;
	} else {
		//cout << "Update sound sensor: " << value << endl;
		pdemo->sensor_sound = value;
	}
}	

void t_read_ultrasonic(void *param)
{
	DemoResource *pdemo = (DemoResource *)param;
	int value = pdemo->ultrasonic_read();

	if(value == -1) {
		cout << "Unable to read ultrasonic sensor" << endl;
	} else {
		//cout << "Update ultrasonic: " << value << endl;
		pdemo->ultrasonic = value;
	}
}	

void t_update_ip_on_lcd(void *param)
{
	DemoResource *pdemo = (DemoResource *)param;
	pdemo->update_ip();
}


void t_write_led_red(void *param)
{
	DemoResource *pdemo = (DemoResource *)param;
	pdemo->led_write_red(pdemo->led_red);
}

void t_write_led_green(void *param)
{
	DemoResource *pdemo = (DemoResource *)param;
	pdemo->led_write_green(pdemo->led_green);
}

void t_write_led_blue(void *param)
{
	DemoResource *pdemo = (DemoResource *)param;
	pdemo->led_write_blue(pdemo->led_blue);
}

void * grovepi_thread(void *param)
{
	void (*w_func)(void *);
	list <void (*)(void *)> read_func;
	list <void (*)(void *)>::iterator iter_f;

	read_func.push_back(t_read_temp);
	read_func.push_back(t_read_humidity);
	read_func.push_back(t_read_light);
	read_func.push_back(t_read_sound);
	read_func.push_back(CheckButtonRepresentation);
	read_func.push_back(t_read_ultrasonic);
	iter_f = read_func.begin();

	//cout << "Function pointer list size: " << read_func.size() << endl;	

	while(1) {
		led_write_lock.lock();
		if(!write_queue.empty()) {
			w_func = write_queue.front();
			w_func(param);
			write_queue.pop();
		}
		led_write_lock.unlock();

		(*iter_f)(param);
		iter_f++;
			
		if(iter_f == read_func.end())
			iter_f = read_func.begin();
	}

	return NULL;
}


// CheckButtonRepresentaion is an observation function,
// which notifies any changes to the resource to stack
// via notifyObservers if button is pressed or released
void CheckButtonRepresentation (void *param)
{
	DemoResource *pdemo = (DemoResource*) param;
	int status;

	if (gObservation) {
		// If under observation if there are any changes to the button resource
		// we call notifyObservors
		
		status = pdemo->button_read();
		if(status == -1) {
			cout << "Button status read failed" << endl;
		}

		cout << "\nButton status: " << status << endl;
		cout << "Current button status: " << pdemo->button << endl;

		if(pdemo->button != status) {
			pdemo->button = status;
			cout << "Notifying observers with resource handle: " << pdemo->button_resourceHandle << endl;

			OCStackResult result = OC_STACK_OK;

			if(isListOfObservers) {
				std::shared_ptr<OCResourceResponse> resourceResponse =
					{std::make_shared<OCResourceResponse>()};

				resourceResponse->setErrorCode(200);
				resourceResponse->setResourceRepresentation(pdemo->get_button(), DEFAULT_INTERFACE);

				result = OCPlatform::notifyListOfObservers(pdemo->button_resourceHandle,
					pdemo->m_interestedObservers, resourceResponse);
			} else {
				result = OCPlatform::notifyAllObservers(pdemo->button_resourceHandle);
			}

			if(OC_STACK_NO_OBSERVERS == result) {
				cout << "No More observers, stopping notifications" << endl;
				gObservation = 0;
			}
		}
	}
}

void * handleSlowResponse (void *param, std::shared_ptr<OCResourceRequest> pRequest)
{
	// This function handles slow response case
	DemoResource* demoPtr = (DemoResource*) param;
	// Induce a case for slow response by using sleep
	std::cout << "SLOW response" << std::endl;
	sleep (10);

	auto pResponse = std::make_shared<OC::OCResourceResponse>();
	pResponse->setRequestHandle(pRequest->getRequestHandle());
	pResponse->setResourceHandle(pRequest->getResourceHandle());
	pResponse->setResourceRepresentation(demoPtr->get_sensor());
	pResponse->setErrorCode(200);
	pResponse->setResponseResult(OC_EH_OK);

	// Set the slow response flag back to false
	isSlowResponse = false;
	OCPlatform::sendResponse(pResponse);

	return NULL;
}

void PrintUsage()
{
	std::cout << std::endl;
	std::cout << "Usage : simpleserver <value>\n";
	std::cout << "    Default - Non-secure resource and notify all observers\n";
	std::cout << "    1 - Non-secure resource and notify list of observers\n\n";
	std::cout << "    2 - Secure resource and notify all observers\n";
	std::cout << "    3 - Secure resource and notify list of observers\n\n";
	std::cout << "    4 - Non-secure resource, GET slow response, notify all observers\n";
}

int wait_for_network(void)
{
	struct ifaddrs *ifaddr, *ifa;
	int s;
	char host[NI_MAXHOST];

	while(1) {
		if (getifaddrs(&ifaddr) == -1) {
			std::cout << "getifaddrs" << std::endl;
			return 1;
		}

		for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
			if (ifa->ifa_addr == NULL)
				continue;  

			s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

			if(((strncmp(ifa->ifa_name, "eth", 3) == 0) || strncmp(ifa->ifa_name, "eno", 3) == 0) && (ifa->ifa_addr->sa_family == AF_INET)) {
				if (s != 0) {
					std::cout << "getnameinfo() failed: " << gai_strerror(s) << std::endl;
				} else {
					std::cout << "Ethernet is up, start demoserver" << std::endl;
					freeifaddrs(ifaddr);
					return 0;
				}
			}

			if(((strncmp(ifa->ifa_name, "wlan", 4) == 0) || strncmp(ifa->ifa_name, "wlo", 3) == 0) && (ifa->ifa_addr->sa_family == AF_INET)) {
				if (s != 0) {
					std::cout << "getnameinfo() failed: " << gai_strerror(s) << std::endl;
				} else {
					std::cout << "Wireless is up, start demoserver" << std::endl;
					freeifaddrs(ifaddr);
					return 0;
				}
			}
		}

		freeifaddrs(ifaddr);
		sleep(1);
	}

	return 1;
}

static FILE* client_open(const char* /*path*/, const char *mode)
{
	return fopen("./oic_svr_db_server.json", mode);
}

int main(int argc, char* argv[])
{
	OCPersistentStorage ps {client_open, fread, fwrite, fclose, unlink };

	if (argc == 1) {
		isListOfObservers = false;
		isSecure = false;
	} else if (argc == 2) {
		int value = atoi(argv[1]);
		switch (value) {
			case 1:
				isListOfObservers = true;
				isSecure = false;
				break;
			case 2:
				isListOfObservers = false;
				isSecure = true;
				break;
			case 3:
				isListOfObservers = true;
				isSecure = true;
				break;
			case 4:
				isSlowResponse = true;
				break;
			default:
				break;
		}
	} else {
		return -1;
	}

	// Create PlatformConfig object
	PlatformConfig cfg {
		OC::ServiceType::InProc,
		OC::ModeType::Server,
		"0.0.0.0", // By setting to "0.0.0.0", it binds to all available interfaces
		0,         // Uses randomly available port
		OC::QualityOfService::LowQos,
		&ps
	};

	OCPlatform::Configure(cfg);
	try {
		// Create the instance of the resource class
		DemoResource myDemo;

		if(myDemo.python_server_connect()) {
			std::cout << "Can not connect to GrovePi python server" << std::endl;
			return 1;
		}

		// Invoke createResource function of class light.
		myDemo.createResource();
		std::cout << "Created resource." << std::endl;
	
		myDemo.update_ip();

		myDemo.start_update_thread();

		//myDemo.addType(std::string("demo.grovepi"));
		//myDemo.addInterface(std::string(LINK_INTERFACE));
		//std::cout << "Added Interface and Type" << std::endl;


		// A condition variable will free the mutex it is given, then do a non-
		// intensive block until 'notify' is called on it.  In this case, since we
		// don't ever call cv.notify, this should be a non-processor intensive version
		// of while(true);
		std::mutex blocker;
		std::condition_variable cv;
		std::unique_lock<std::mutex> lock(blocker);
		std::cout <<"Waiting" << std::endl;
		cv.wait(lock, []{return false;});
	}
	catch(OCException &e) {
		std::cout << "OCException in main : " << e.what() << endl;
	}

	// No explicit call to stop the platform.
	// When OCPlatform::destructor is invoked, internally we do platform cleanup

	return 0;
}
