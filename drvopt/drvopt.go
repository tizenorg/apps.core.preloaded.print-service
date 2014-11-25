/*
*	Printservice
*
* Copyright (c) 2012-2013 Samsung Electronics Co., Ltd.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*                   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.*
*
*/

package main

import (
	"./drv"
	"flag"
	"fmt"
	"os"
	"runtime/pprof"
	"strings"
	"io/ioutil"
	"path"
	"os/exec"
)

var (
	indir       string
	inopt       string
	outfile     string
	show_help   bool
	id          string
	cpuprofile  string
	memprofile  string
	listopt     string
	nameopt     string
	listprod    string
	prodopt     string
)

func init() {
	flag.StringVar(&indir, "d", "", "input drv directory")
	flag.StringVar(&inopt, "q", "", "input opt file")
	flag.StringVar(&outfile, "o", "stripped.drv", "output file")
	flag.StringVar(&cpuprofile, "p", "", "write cpu profile to file")
	flag.StringVar(&memprofile, "m", "", "write memory profile to this file")
	flag.StringVar(&listopt, "l", "", "list available printers by model")
	flag.StringVar(&listprod, "c", "", "list available printers by product")
	flag.StringVar(&nameopt, "n", "", "print drv by model name")
	flag.StringVar(&prodopt, "x", "", "print drv by product name")
}

func main() {
	flag.Parse()

	if listopt != "" {
		drvm := drv.LoadDrvm(listopt)
		drvm.ListModels()
		return
	}

	if listprod != "" {
		drvm := drv.LoadDrvm(listprod)
		drvm.ListProducts()
	}

	if cpuprofile != "" {
		f, err := os.Create(cpuprofile)
		if err != nil {
			fmt.Print(err)
		}
		pprof.StartCPUProfile(f)
		defer pprof.StopCPUProfile()
	}

	if memprofile != "" {
		f, err := os.Create(memprofile)
		if err != nil {
			fmt.Print(err)
		}
		pprof.WriteHeapProfile(f)
		defer f.Close()
	}

	if indir != "" {
		drvm := new(drv.Drvm)
		files, err := ioutil.ReadDir(indir)
		if err != nil {
			fmt.Println(err)
			return
		}
		for _, fp := range files {
			fn := fp.Name()
			fmt.Println("\nfile:", fn)
			if !strings.HasSuffix(fn, ".ppd") { continue; }
			fn = path.Join(indir, fn)
			cmd := exec.Command("ppdi", "-o", "/tmp/tmp.drv", fn)
			err := cmd.Run()
			if err != nil {
			        fmt.Println(err)
		        	return
			}
			err = drvm.ExtendDrvm("/tmp/tmp.drv")
			if err != nil {
			        fmt.Println(err)
		        	return
			}
			cmd = exec.Command("rm", "/tmp/tmp.drv")
			err = cmd.Run()
		}

		drvm.SaveDrvm(outfile)
	}
	return
}
