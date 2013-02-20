import os, time
import bliss.saga as saga
import glob

def main():
	
	try:
		os.chdir("numbers")
		for files in glob.glob("*.txt"):

			#create a job service
			js = saga.job.Service("pbs+ssh://vshah505@india.futuregrid.org")

			#describe our job
			jd = saga.job.Description()
			jd.wall_time_limit  = 60
			jd.total_cpu_count = 1
			jd.working_directory = '/N/u/vshah505/OS/project1/'
			jd.executable = '/bin/bash'
			jd.arguments = ['testscript_r.sh', files]
			jd.error = "err.stderr"

			#create the job (state:New)
			job = js.create_job(jd)
			job.run()

	except saga.Exception, ex:
		print "An error occured during job execution: %s" % (str(ex))
		sys.exit(-1)

if __name__ == "__main__":
	main()