/*
*	Printservice
*
* Copyright 2012  Samsung Electronics Co., Ltd

* Licensed under the Flora License, Version 1.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at

* http://floralicense.org/license/

* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

package main

import (
	"./drv"
	"flag"
	"fmt"
	"os"
	"os/exec"
	"runtime/pprof"
)

var (
	infile    string
	outfile   string
	glob      string
	show_help bool
	cpuprofile string
)

func init() {
	flag.StringVar(&infile, "i", "stripped.drv", "input drv file")
	flag.StringVar(&glob, "r", "", "model search pattern")
	flag.StringVar(&cpuprofile, "p", "", "write cpu profile to file")
}

func main() {
	flag.Parse()

	if cpuprofile != "" {
        f, err := os.Create(cpuprofile)
        if err != nil {
            fmt.Print(err)
        }
        pprof.StartCPUProfile(f)
        defer pprof.StopCPUProfile()
    }
	
	if glob == "" {
		flag.Usage()
		return
	}

	buf, err := drv.Read(infile)
	if err != nil {
		fmt.Println(err)
		return
	}
	
	model, err := drv.Find_model(buf, glob)
	if err != nil {
		fmt.Println(err)
		return
	}
	
	if outfile == "" {
		outfile = glob + ".drv"
	}
	
	fd, err := os.Create(outfile)
	if err != nil {
		fmt.Println(err)
		return
	}
	defer os.Remove(outfile)
	defer fd.Close()
	
	_, err = fd.Write([]byte(model))
	if err != nil {
		fmt.Println(err)
		return
	}
	
	cmd := exec.Command("ppdc", "-d", ".", outfile)
	err = cmd.Run()
	if err != nil {
    	fmt.Println("ppdc:", err)
	}
	
	return
}
